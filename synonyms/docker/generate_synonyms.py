import os
from tqdm import tqdm
from sentence_transformers import SentenceTransformer, util
from keybert import KeyBERT
import nltk
import numpy as np

# Download NLTK data (first run only)
nltk.download('punkt')
nltk.download('stopwords')

# === CONFIGURATION ===
TEXT_FOLDER = "texts"       # Folder with .txt files
THRESHOLD = 0.65            # Similarity threshold
OUTPUT_FILE = "synonyms.txt"

# === INITIALIZE MODELS ===
print("🔹 Loading models...")
sbert_model = SentenceTransformer('all-MiniLM-L6-v2')
kw_model = KeyBERT(model=sbert_model)
print("✅ Models loaded.\n")

# === LOAD TEXTS WITH PROGRESS ===
print(f"📂 Loading text files from '{TEXT_FOLDER}'...")
texts = []
file_list = os.listdir(TEXT_FOLDER)
with tqdm(file_list, desc="Reading files", unit="file") as pbar:
    for filename in pbar:
        if filename.endswith(".txt"):
            path = os.path.join(TEXT_FOLDER, filename)
            with open(path, "r", encoding="utf-8") as f:
                texts.append(f.read())
        pbar.set_postfix({"loaded_files": len(texts)})
print(f"✅ Loaded {len(texts)} text files.\n")

# === EXTRACT KEYPHRASES WITH PROGRESS ===
print("🔹 Extracting keyphrases...")
all_phrases = set()
with tqdm(texts, desc="Extracting phrases", unit="text") as pbar:
    for text in pbar:
        keywords = kw_model.extract_keywords(
            text,
            keyphrase_ngram_range=(1, 3),
            stop_words='english',
            top_n=10
        )
        for phrase, _ in keywords:
            all_phrases.add(phrase.lower())
        pbar.set_postfix({"unique_phrases": len(all_phrases)})
phrases = list(all_phrases)
print(f"✅ Extracted {len(phrases)} unique candidate phrases.\n")

# === COMPUTE EMBEDDINGS & SIMILARITY ===
print("🔹 Computing embeddings...")
embeddings = sbert_model.encode(phrases, convert_to_tensor=True)
cosine_scores = util.pytorch_cos_sim(embeddings, embeddings).cpu().numpy()
print("✅ Embeddings computed.\n")

# === FIND SYNONYMS WITH PROGRESS ===
print("🔹 Grouping similar phrases...")
synonym_groups = {}
with tqdm(phrases, desc="Finding synonyms", unit="phrase") as pbar:
    for i, term in enumerate(pbar):
        similar_indices = np.where(cosine_scores[i] > THRESHOLD)[0]
        similar_terms = [
            (phrases[j], float(cosine_scores[i][j]))
            for j in similar_indices if j != i
        ]
        if similar_terms:
            synonym_groups[term] = sorted(similar_terms, key=lambda x: -x[1])
        pbar.set_postfix({"groups_found": len(synonym_groups)})

# === DEDUPLICATE OVERLAPS WITH PROGRESS ===
print("🔹 Deduplicating overlapping groups...")
final_groups = {}
seen = set()
with tqdm(synonym_groups.items(), desc="Deduplicating", unit="group") as pbar:
    for main, group in pbar:
        if main not in seen:
            children = [t for t, _ in group if t not in seen]
            seen.update(children + [main])
            final_groups[main] = group
        pbar.set_postfix({"final_groups": len(final_groups)})

# === FORMAT OUTPUT WITH WEIGHTS & PROGRESS ===
print("🔹 Formatting output...")
output_lines = []
with tqdm(final_groups.items(), desc="Formatting lines", unit="group") as pbar:
    for main, group in pbar:
        if group:
            children_fmt = []
            for term, score in group:
                if score >= 1.0:
                    children_fmt.append(term)  # omit weight if 1
                else:
                    children_fmt.append(f"{term}:{score:.2f}"[1:])  # drop leading zero
            line = f"# {main} = " + "+".join(children_fmt)
            output_lines.append(line)
        pbar.set_postfix({"lines_written": len(output_lines)})

# === SAVE OUTPUT ===
with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    f.write("\n".join(output_lines))

print(f"\n✅ Synonym list written to '{OUTPUT_FILE}'")

