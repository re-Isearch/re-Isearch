import os
from sentence_transformers import SentenceTransformer, util
from keybert import KeyBERT
import nltk
import numpy as np

# Download NLTK data
nltk.download('punkt')
nltk.download('stopwords')

# === CONFIGURATION ===
TEXT_FOLDER = "texts"   # Folder with .txt files
THRESHOLD = 0.65        # Similarity threshold
OUTPUT_FILE = "synonyms.txt"

# === INITIALIZE MODELS ===
sbert_model = SentenceTransformer('all-MiniLM-L6-v2')
kw_model = KeyBERT(model=sbert_model)

# === STEP 1: LOAD TEXT CORPUS ===
texts = []
for filename in os.listdir(TEXT_FOLDER):
    if filename.endswith(".txt"):
        path = os.path.join(TEXT_FOLDER, filename)
        with open(path, "r", encoding="utf-8") as f:
            texts.append(f.read())

print(f"Loaded {len(texts)} text files from '{TEXT_FOLDER}'")

# === STEP 2: EXTRACT CANDIDATE PHRASES ===
all_phrases = set()
for text in texts:
    keywords = kw_model.extract_keywords(
        text,
        keyphrase_ngram_range=(1, 3),
        stop_words='english',
        top_n=10
    )
    for phrase, _ in keywords:
        all_phrases.add(phrase.lower())

phrases = list(all_phrases)
print(f"Extracted {len(phrases)} unique candidate phrases")

# === STEP 3: EMBED AND COMPUTE SIMILARITIES ===
embeddings = sbert_model.encode(phrases, convert_to_tensor=True)
cosine_scores = util.pytorch_cos_sim(embeddings, embeddings).cpu().numpy()

# === STEP 4: BUILD SYNONYM GROUPS ===
synonym_groups = {}
for i, term in enumerate(phrases):
    similar_indices = np.where(cosine_scores[i] > THRESHOLD)[0]
    similar_terms = [
        (phrases[j], float(cosine_scores[i][j]))
        for j in similar_indices if j != i
    ]
    if similar_terms:
        sorted_terms = sorted(similar_terms, key=lambda x: -x[1])
        synonym_groups[term] = sorted_terms

# === STEP 5: DEDUPLICATE OVERLAPS ===
final_groups = {}
seen = set()
for main, group in synonym_groups.items():
    if main not in seen:
        children = [t for t, _ in group if t not in seen]
        seen.update(children + [main])
        final_groups[main] = group

# === STEP 6: FORMAT OUTPUT ===
output_lines = []
for main, group in final_groups.items():
    if group:
        # Convert similarity weights to integers (0–100)
        children_fmt = [
            f"{term}:{int(score * 100)}" for term, score in group
        ]
        line = f"# {main} = " + "+".join(children_fmt)
        output_lines.append(line)

formatted_output = "\n".join(output_lines)

# === STEP 7: SAVE OUTPUT ===
with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    f.write(formatted_output)

print(f"✅ Synonym list written to '{OUTPUT_FILE}'")

