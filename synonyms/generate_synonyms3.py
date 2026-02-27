import os
from tqdm import tqdm
from sentence_transformers import SentenceTransformer, util
from keybert import KeyBERT
import nltk
import numpy as np

# Download NLTK data
nltk.download('punkt')
nltk.download('stopwords')

# === CONFIGURATION ===
TEXT_FOLDER = "texts"   # Folder containing .txt files
THRESHOLD = 0.65        # Similarity threshold (0.6–0.8 typical)
OUTPUT_FILE = "synonyms.txt"

# === INITIALIZE MODELS ===
print("🔹 Loading models...")
sbert_model = SentenceTransformer('all-MiniLM-L6-v2')
kw_model = KeyBERT(model=sbert_model)
print("✅ Models loaded.")

# === STEP 1: LOAD TEXT CORPUS ===
texts = []
print(f"\n📂 Loading text files from '{TEXT_FOLDER}'...")
for filename in tqdm(os.listdir(TEXT_FOLDER), desc="Reading files"):
    if filename.endswith(".txt"):
        path = os.path.join(TEXT_FOLDER, filename)
        with open(path, "r", encoding="utf-8") as f:
            texts.append(f.read())

print(f"✅ Loaded {len(texts)} text files.\n")

# === STEP 2: EXTRACT CANDIDATE PHRASES ===
print("🔹 Extracting keyphrases with KeyBERT...")
all_phrases = set()
for text in tqdm(texts, desc="Extracting phrases"):
    keywords = kw_model.extract_keywords(
        text,
        keyphrase_ngram_range=(1, 3),
        stop_words='english',
        top_n=10
    )
    for phrase, _ in keywords:
        all_phrases.add(phrase.lower())

phrases = list(all_phrases)
print(f"✅ Extracted {len(phrases)} unique candidate phrases.\n")

# === STEP 3: EMBED PHRASES ===
print("🔹 Computing embeddings...")
embeddings = sbert_model.encode(phrases, convert_to_tensor=True)
cosine_scores = util.pytorch_cos_sim(embeddings, embeddings).cpu().numpy()
print("✅ Embeddings computed.\n")

# === STEP 4: BUILD SYNONYM GROUPS WITH PROGRESS ===
print("🔹 Grouping similar phrases...")
synonym_groups = {}
for i, term in enumerate(tqdm(phrases, desc="Finding synonyms")):
    similar_indices = np.where(cosine_scores[i] > THRESHOLD)[0]
    similar_terms = [
        (phrases[j], float(cosine_scores[i][j]))
        for j in similar_indices if j != i
    ]
    if similar_terms:
        sorted_terms = sorted(similar_terms, key=lambda x: -x[1])
        synonym_groups[term] = sorted_terms

print(f"✅ Found {len(synonym_groups)} synonym groups.\n")

# === STEP 5: DEDUPLICATE OVERLAPS ===
print("🔹 Deduplicating overlapping groups...")
final_groups = {}
seen = set()
for main, group in tqdm(synonym_groups.items(), desc="Deduplicating"):
    if main not in seen:
        children = [t for t, _ in group if t not in seen]
        seen.update(children + [main])
        final_groups[main] = group

print(f"✅ Final unique groups: {len(final_groups)}\n")

# === STEP 6: FORMAT OUTPUT ===
print("🔹 Formatting output...")
output_lines = []
for main, group in tqdm(final_groups.items(), desc="Formatting lines"):
    if group:
        children_fmt = [
            f"{term}:{int(score * 100)}" for term, score in group
        ]
        line = f"# {main} = " + "+".join(children_fmt)
        output_lines.append(line)

formatted_output = "\n".join(output_lines)

# === STEP 7: SAVE OUTPUT ===
with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    f.write(formatted_output)

print(f"\n✅ Synonym list written to '{OUTPUT_FILE}'")

