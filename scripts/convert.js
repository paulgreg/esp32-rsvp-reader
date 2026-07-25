#!/usr/bin/env node

import AdmZip from "adm-zip";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import * as cheerio from "cheerio";

const __dirname = path.dirname(fileURLToPath(import.meta.url));

function escapeAccents(text) {
  let result = "";
  for (const char of text) {
    const code = char.codePointAt(0);

    if (code <= 127) {
      result += char;
    } else if (code <= 255) {
      // Keep Latin-1 characters as single-byte code points for Arduino rendering.
      result += String.fromCharCode(code);
    } else {
      // Drop characters outside Latin-1.
      continue;
    }
  }

  return result;
}

function normalizeLineBreaks(text) {
  text = text.replaceAll("\r\n", "\n");
  return text.replaceAll("\n\n\n", "\n\n");
}

function removeLineBreaksInSentences(text) {
  const paragraphs = text.split(/\n\s*\n+/);
  return paragraphs
    .map((para) => {
      return para.replaceAll(/\s*\n\s*/g, " ").trim();
    })
    .join("\n\n");
}

function normalizePunctuation(text) {
  return text
    .replaceAll("—", "-")
    .replaceAll("–", "-")
    .replaceAll("„", '"')
    .replaceAll("“", '"')
    .replaceAll("”", '"')
    .replaceAll("‘", "'")
    .replaceAll("’", "'")
    .replaceAll("´", "'")
    .replaceAll("‹", "<")
    .replaceAll("›", ">")
    .replaceAll("…", "...");
  //.replaceAll("¿", "?")
  //.replaceAll("¡", "!");
}

function extractTextFromHtml(html) {
  const $ = cheerio.load(html, { decodeEntities: false });

  // Remove script and style blocks
  $("script, style").remove();

  // Extract text from each element individually to preserve structure
  const elements = $("body").children().get();
  const texts = [];

  for (const el of elements) {
    const text = $(el).text();
    if (text.trim()) {
      // Add paragraph break before headings and block elements
      const tagName = $(el).prop("tagName");
      const isHeading = tagName && /^h[1-6]$/.test(tagName);
      const isBlock = tagName && /^(div|p|h[1-6]|li)$/.test(tagName);

      if (isHeading || isBlock) {
        texts.push("\n\n" + text.trim());
      } else {
        texts.push(text.trim());
      }
    }
  }

  // Normalize whitespace - collapse multiple spaces
  const result = texts.join("\n\n").replaceAll(/\s+/g, " ");

  return result
    .replaceAll("&nbsp;", " ")
    .replaceAll("&amp;", "&")
    .replaceAll("&lt;", "<")
    .replaceAll("&gt;", ">")
    .replaceAll("&quot;", '"')
    .replaceAll(/&#(\d+);/g, (match, dec) => String.fromCodePoint(dec))
    .replaceAll(/&#x([0-9a-fA-F]+);/g, (match, hex) => String.fromCodePoint(Number.parseInt(hex, 16)));
}

function cleanText(text) {
  text = normalizeLineBreaks(text);
  text = removeLineBreaksInSentences(text);
  text = normalizePunctuation(text);
  return escapeAccents(text.trim());
}

function cleanMetadataField(text) {
  return escapeAccents(normalizePunctuation(text)).trim();
}

function extractEpubMetadata(opfContent) {
  // Get the LAST dc:title (some EPUBs have multiple, last one is the real title)
  const titleMatches = opfContent.matchAll(/<dc:title[^>]*>(.*?)<\/dc:title>/gi);
  let title = "Unknown Title";
  for (const match of titleMatches) {
    title = match[1].trim();
  }

  const authorMatch = opfContent.match(/<dc:creator[^>]*>(.*?)<\/dc:creator>/i);
  const author = authorMatch ? authorMatch[1].trim() : "Unknown Author";

  return {
    title: cleanMetadataField(title),
    author: cleanMetadataField(author),
  };
}

function extractEpubContent(epubPath) {
  return new Promise((resolve, reject) => {
    try {
      const zip = new AdmZip(epubPath);
      const entries = zip.getEntries();

      // Find content.opf
      let opfEntry = null;
      for (const entry of entries) {
        if (entry.entryName.endsWith("content.opf")) {
          opfEntry = entry;
          break;
        }
      }

      if (!opfEntry) {
        reject(new Error("No content.opf found in EPUB"));
        return;
      }

      const opfContent = opfEntry.getData().toString("utf8");
      const metadata = extractEpubMetadata(opfContent);

      // Extract XHTML files from manifest
      const manifestMatch = opfContent.match(/<manifest[^>]*>(.*?)<\/manifest>/is);
      if (!manifestMatch) {
        reject(new Error("No manifest found in OPF"));
        return;
      }

      const manifest = manifestMatch[1];
      const xhtmlItems = [];

      const xhtmlRegex = /<item[^>]*href="([^"]+\.xhtml)"[^>]*>/gi;
      let match;
      while ((match = xhtmlRegex.exec(manifest)) !== null) {
        xhtmlItems.push(match[1]);
      }

      let allText = "";
      for (const href of xhtmlItems) {
        const entry = entries.find((e) => e.entryName.endsWith(href));
        if (!entry) continue;

        const entryContent = entry.getData().toString("utf8");
        const textContent = extractTextFromHtml(entryContent);

        if (textContent.trim()) {
          allText += textContent + "\n\n";
        }
      }

      const cleanedText = cleanText(allText);
      resolve({ text: cleanedText, metadata });
    } catch (error) {
      reject(error);
    }
  });
}

function processTxtFile(filePath) {
  let content = fs.readFileSync(filePath, "utf-8");
  // Normalize line endings to Unix format (\n)
  content = content.replaceAll("\r\n", "\n").replaceAll("\r", "\n");
  return {
    text: cleanText(content),
    metadata: { title: "Unknown Title", author: "Unknown Author" },
  };
}

function createOutputContent(_metadata, textContent) {
  // ignore meta data for now
  // return `TITLE=${metadata.title}\nAUTHOR=${metadata.author}\n---\n${textContent}`;
  return textContent;
}

async function main() {
  const args = process.argv.slice(2);

  if (args.length < 1) {
    console.error("Usage: node convert.js <input.epub|input.txt> [output.txt]");
    console.error("");
    console.error("Converts EPUB or TXT files to e-reader format.");
    console.error("");
    console.error("Accent characters are converted to Latin-1 bytes for Arduino display.");
    console.error("Line breaks within sentences are removed, but paragraphs are preserved.");
    process.exit(1);
  }

  const inputPath = args[0];
  const outputPath = args[1] || path.basename(inputPath, path.extname(inputPath)) + ".txt";

  if (!fs.existsSync(inputPath)) {
    console.error(`Error: File not found: ${inputPath}`);
    process.exit(1);
  }

  const ext = path.extname(inputPath).toLowerCase();

  try {
    let result = { text: "", metadata: { title: "Unknown Title", author: "Unknown Author" } };

    if (ext === ".epub") {
      result = await extractEpubContent(inputPath);
    } else if (ext === ".txt") {
      result = processTxtFile(inputPath);
      const filename = path.basename(inputPath, ".txt");
      const title = filename.replace(/[-_]/g, " ").replace(/\b\w/g, (l) => l.toUpperCase());
      result.metadata = {
        title,
        author: result.metadata.author,
      };
    } else {
      console.error(`Error: Unsupported file format: ${ext}`);
      process.exit(1);
    }

    const output = createOutputContent(result.metadata, result.text);
    fs.writeFileSync(outputPath, output, "latin1");

    console.log(`Converted: ${inputPath}`);
    console.log(`Output: ${outputPath}`);
    console.log(`Title: ${result.metadata.title}`);
    console.log(`Author: ${result.metadata.author}`);
    console.log(`Content length: ${result.text.length} characters`);
  } catch (error) {
    console.error(`Error: ${error.message}`);
    process.exit(1);
  }
}

main();
