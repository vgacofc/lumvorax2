sudo xmrig --config=config.json

xT

npm install supermemory


import Supermemory from 'supermemory';

const client = new Supermemory({
  apiKey: process.env.SUPERMEMORY_API_KEY,  // Default, can be omitted
});

// Add a memory
await client.add({ content: "Meeting notes from Q1 planning", containerTags: ["user_123"] });

// Search memories
const response = await client.search.documents({
  q: "planning notes",
  containerTags: ["user_123"]
});
console.log(response.results);

// Get user profile
const profile = await client.profile({ containerTag: "user_123" });
console.log(profile.profile.static);
console.log(profile.profile.dynamic);

// Add with metadata
await client.add({
  content: "Technical design doc",
  containerTags: ["user_123"],
  metadata: { category: "engineering", priority: "high" }
});

// Search with filters
const results = await client.search.documents({
  q: "design document",
  containerTags: ["user_123"],
  filters: {
    AND: [
      { key: "category", value: "engineering" }
    ]
  }
});

// List documents
const docs = await client.documents.list({ containerTags: ["user_123"], limit: 10 });

// Delete a document
await client.documents.delete({ docId: "doc_123" });