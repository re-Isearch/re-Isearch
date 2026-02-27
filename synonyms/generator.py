from sentence_transformers import SentenceTransformer, util
from keybert import KeyBERT
from nltk.corpus import stopwords
import nltk
import numpy as np

#Example output:
#spatial=geospatial+geographic+terrestrial  # avg sim=0.77
#land use=land cover+land characterization+land surface+ownership property  # avg sim=0.81
#wetlands=wet land+nwi+hydric soil+inundated  # avg sim=0.73
#hydrography=stream+river+spring+lake+pond+aqueduct+siphon+well  # avg sim=0.76
#hypsography=elevation+relief+topography+contour  # avg sim=0.74


#⚙️ Notes
#Adjust the threshold (0.65–0.8) for tighter or looser groupings.
#You can replace texts with any collection of documents.
#The comment at the end of each line shows how semantically cohesive that synonym group is.


# Download required NLTK data
nltk.download('punkt')
nltk.download('stopwords')

# Initialize models
sbert_model = SentenceTransformer('all-MiniLM-L6-v2')
kw_model = KeyBERT(model=sbert_model)

# Example corpus (replace with your own)
texts = [
    "Land use and land cover are key concepts in environmental geography.",
    "Wetlands are areas of hydric soils and inundated surfaces.",
    "Hydrography includes rivers, lakes, springs, ponds, aqueducts, and wells.",
    "Hypsography refers to elevation, contour lines, and terrain relief.",
    "Spatial and geospatial data are used to describe geographic features of the Earth.",
]

# Step 1: Extract multi-word keyphrases
all_phrases = set()
for text in texts:
    keywords = kw_model.extract_keywords(
        text, keyphrase_ngram_range=(1, 3), stop_words='english', top_n=10
    )
    for phrase, _ in keywords:
        all_phrases.add(phrase.lower())

phrases = list(all_phrases)

# Step 2: Embed all phrases
embeddings = sbert_model.encode(phrases, convert_to_tensor=True)

# Step 3: Compute cosine similarity matrix
cosine_scores = util.pytorch_cos_sim(embeddings, embeddings).cpu().numpy()

# Step 4: Group similar phrases
threshold = 0.65  # tune this based on corpus
synonym_groups = {}

for i, term in enumerate(phrases):
    similar_indices = np.where(cosine_scores[i] > threshold)[0]
    similar_terms = [
        (phrases[j], float(cosine_scores[i][j]))
        for j in similar_indices if j != i
    ]
    if similar_terms:
        sorted_terms = sorted(similar_terms, key=lambda x: -x[1])
        synonym_groups[term] = sorted_terms

# Step 5: Deduplicate overlapping groups
final_groups = {}
seen = set()

for main, group in synonym_groups.items():
    if main not in seen:
        children = [t for t, _ in group if t not in seen]
        seen.update(children + [main])
        final_groups[main] = group

# Step 6: Build formatted output
output_lines = []
for main, group in final_groups.items():
    if group:
        children = [t for t, _ in group]
        avg_sim = np.mean([s for _, s in group])
        line = f"{main}=" + "+".join(children) + f"  # avg sim={avg_sim:.2f}"
        output_lines.append(line)

formatted_output = "\n".join(output_lines)
print(formatted_output)

# Optionally save to file
with open("synonyms.txt", "w") as f:
    f.write(formatted_output)

