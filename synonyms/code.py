from sentence_transformers import SentenceTransformer, util
from nltk.corpus import stopwords
from nltk.tokenize import word_tokenize
import nltk
import itertools
import numpy as np

# Download required NLTK data
nltk.download('punkt')
nltk.download('stopwords')

# Load model
model = SentenceTransformer('all-MiniLM-L6-v2')

# Sample text corpus
texts = [
    "The car was fast and powerful.",
    "The automobile had incredible speed.",
    "A quick vehicle zoomed past us."
]

# 1. Tokenize and clean words
stop_words = set(stopwords.words('english'))
words = set()
for text in texts:
    tokens = word_tokenize(text.lower())
    words.update([t for t in tokens if t.isalpha() and t not in stop_words])

words = list(words)

# 2. Get embeddings
embeddings = model.encode(words, convert_to_tensor=True)

# 3. Compute similarity matrix
cosine_scores = util.pytorch_cos_sim(embeddings, embeddings).cpu().numpy()

# 4. Extract synonyms with weights
synonym_dict = {}
threshold = 0.6  # similarity threshold (tune as needed)

for i, word in enumerate(words):
    similar_indices = np.where(cosine_scores[i] > threshold)[0]
    similar_words = [
        (words[j], float(cosine_scores[i][j]))
        for j in similar_indices if j != i
    ]
    if similar_words:
        synonym_dict[word] = sorted(similar_words, key=lambda x: -x[1])

# 5. Display results
for word, sims in synonym_dict.items():
    print(f"\n{word}:")
    for syn, weight in sims:
        print(f"  - {syn} ({weight:.2f})")

