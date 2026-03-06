#!/usr/bin/env python3
"""
Profile Generator for AKM - 500 Profiles using wordlist_512_ascii.txt
Each profile uses words from the official 512-word list with unique logic
"""

import os
import random
import hashlib
from datetime import datetime

# Ensure output directory exists
OUTPUT_DIR = "akm_profile"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Read the official 512 wordlist
WORDLIST_FILE = "akm/wordlist_512_ascii.txt"
OFFICIAL_WORDS = []

with open(WORDLIST_FILE, 'r', encoding='utf-8') as f:
    for line in f:
        word = line.strip()
        if word and not word.startswith('#'):
            OFFICIAL_WORDS.append(word)

print(f"Loaded {len(OFFICIAL_WORDS)} words from {WORDLIST_FILE}")
assert len(OFFICIAL_WORDS) == 512, f"Expected 512 words, got {len(OFFICIAL_WORDS)}"

# Profile name components - Romanian-inspired to match wordlist theme
ADJECTIVES = [
    "rapid", "lent", "puternic", "slab", "luminos", "intunecat", 
    "vechi", "nou", "simplu", "complex", "pur", "mixinat",
    "secret", "deschis", "ascuns", "vizibil", "tacut", "zgomotos",
    "cald", "rece", "dulce", "amar", "moale", "tare",
    "inalt", "jos", "lat", "ingust", "plin", "gol",
    "curat", "murdar", "usor", "greu", "subtire", "gros",
    "fragil", "rezistent", "flexibil", "rigid", "neted", "dureros",
    "roscat", "albastru", "verde", "galben", "mov", "portocaliu",
    "alb", "negru", "gri", "maro", "roz", "violet",
    "stravezi", "opac", "lucios", "mat", "polisat", "brut",
    "primaver", "vara", "toamna", "iarna", "ziua", "noaptea",
    "dimineata", "amiaza", "seara", "miezul", "rasarit", "apus",
    "vesel", "trist", "calm", "nervos", "relaxat", "tensionat",
    "prietenos", "dusmanos", " bland", "aspru", "gentil", "violent",
    "sănătos", "bolnav", "proaspăt", "stale", "dornic", "plictisit",
    "bogat", "sărac", "scump", "ieftin", "valoros", "fără",
    "magic", "real", "fals", "autentic", "artificial", "natural",
    "modern", "antic", "viitor", "trecut", "prezent", "etern",
    "infinit", "finit", "limitat", "nelimitat", "perfect", "imperfect",
    "exact", "aproximativ", "precis", "vag", "clar", "confuz",
    "logic", "emotional", "rational", "intuitiv", "stiintific", "artistic",
    "tehnic", "creativ", "practic", "teoretic", "util", "inutil",
    "activ", "pasiv", "dinamic", "static", "mobil", "imobil",
    "central", "periferic", "interior", "exterior", "superior", "inferior",
    "bun", "rău", "corect", "gresit", "just", "injust",
    "moral", "imoral", "legal", "ilegal", "permis", "interzis",
    "sigur", "periculos", "protectiv", "dăunător", "vindecător", "toxic",
    "energic", "obosit", "vital", "letargic", "alert", "somnolent"
]

NOUNS = [
    "cipher", "cheie", "lacat", "seif", "cufar", "cutie", "container",
    "scut", "armura", "pavaza", "protector", "gardian", "veghetor", "paznic",
    "sabie", "spada", "lance", "sageata", "tun", "pistol", "arc",
    "nucleu", "centru", "miez", "inima", "radacina", "baza", "fundatie",
    "matrice", "grila", "retEA", "panza", "tesatura", "structura",
    "cod", "script", "text", "simbol", "runic", "marca", "semn",
    "scanteie", "flacara", "strălucire", "raza", "lumină", "glow", "brilliant",
    "puls", "bataie", "ritm", "tempo", "cadenta", "vibratie",
    "val", "unda", "tsunami", "torent", "current", "fluviu", "râu",
    "furtuna", "tunet", "fulger", "vijelie", "vant", "briza", "uragan",
    "flacara", "foc", "incendiu", "scanteie", "cărbune", "cenusa", "fum",
    "bruma", "chiciura", "zapada", "grindina", "lapovita", "ploaie", "roua",
    "umbra", "intuneric", "noapte", "abis", "hău", "prăpastie", "gol",
    "lumină", "stralucire", "aurora", "curcubeu", "spectru", "halo", "coroana",
    "stea", "soare", "luna", "planeta", "terra", "galaxie", "univers",
    "nova", "nebula", "quasar", "pulsar", "cometa", "meteorit", "asteroid",
    "lup", "tigru", "leu", "vultur", "soim", "corb", "bufnita",
    "dragon", "balaur", "monstru", "fiara", "creatura", "entitate", "fiinta",
    "titan", "gigant", "colo", "leviatan", "behemoth", "hidra", "fenix",
    "oracol", "profet", "vazator", "intelept", "magician", "vrajitor", "vraci",
    "cavaler", "razboinic", "luptator", "soldat", "aparator", "erou",
    "cale", "drum", "poteca", "ruta", "traseu", "directie", "sens",
    "poarta", "usa", "portal", "intrare", "acces", "trecere", "pasaj",
    "turn", "castel", "fortareata", "cetate", "bastion", "reduta", "donjon",
    "regat", "imperiu", "taram", "teritoriu", "provincie", "stat", "natiune",
    "orizont", "granița", "limita", "margine", "perimetru", "contur", "bordura",
    "sursa", "origine", "radacina", "baza", "fundatie", "pamant", "sol",
    "varf", "culme", "pisc", "apogeum", "zenit", "acme", "maxim",
    "motor", "mecanism", "dispozitiv", "aparat", "instrument", "unealta", "unealta",
    "algoritm", "procedura", "proces", "metoda", "tehnica", "sistem", "schema",
    "patern", "design", "structura", "cadru", "schelet", "retea", "organizare",
    "echilibru", "armonie", "simetrie", "proportie", "raport", "masura", "scala",
    "sine", "ego", "identitate", "personalitate", "caracter", "temperament",
    "natura", "fire", "inclinatie", "pasiune", "entuziasm", "emotie", "sentiment",
    "afectiune", "dragoste", "adorare", "cult", "venerare", "respect", "onoare",
    "demnitate", "mandrie", "modestie", "smerenie", "umilinta", "aroganta",
    "egoism", "altruism", "generozitate", "bunatate", "amabilitate", "politet",
    "cuviinta", "decenta", "corectitudine", "integritate", "onestitate", "sinceritate",
    "adevar", "transparenta", "claritate", "puritate", "impuritate", "poluare"
]

# Different logic types for unique profiles
LOGIC_TYPES = [
    "sequential_hex",      # Sequential hex values
    "repeating_nibble",    # Repeating nibbles (AAAA, BBBB)
    "alternating",         # Alternating (ABAB, 1212)
    "mirror",              # Mirror (ABBA, 1221)
    "fibonacci_based",     # Based on fibonacci numbers
    "prime_based",         # Based on prime numbers
    "palindrome",          # True palindrome
    "word_index_based",    # Based on word index in list
    "hash_derived",        # Derived from SHA256
    "position_shift",      # Shift based on position
    "xor_pattern",         # XOR-based patterns
    "addition_chain",      # Addition chain pattern
    "multiplication",      # Multiplicative pattern
    "exponential",         # Exponential pattern
    "geometric",           # Geometric progression
    "bitwise_complement",  # Bitwise complement
    "rotation",            # Bit rotation
    "shuffle",             # Shuffled patterns
    "interleave",          # Interleaved values
    "matrix_based",        # Matrix-based patterns
    "cyclic",              # Cyclic patterns
    "fractal",             # Fractal-like patterns
    "chaos",               # Chaotic patterns
    "symmetric",           # Symmetric patterns
    "asymmetric"           # Asymmetric patterns
]

def get_word_by_deterministic_index(index, profile_seed):
    """Get a word from the official list using deterministic selection"""
    idx = (index + profile_seed) % 512
    return OFFICIAL_WORDS[idx]

def generate_hex_by_logic(logic_type, seed, word_index, word=None):
    """Generate hex value based on specific logic type"""
    random.seed(seed)
    
    if logic_type == "sequential_hex":
        # Sequential pattern based on seed and index
        base = (seed * 1000 + word_index) % 0xFFFFFFFF
        return f"{base:08x}"
    
    elif logic_type == "repeating_nibble":
        # Repeating nibble (e.g., aaaaaaaa, 44444444)
        nibble = (seed + word_index) % 16
        return f"{nibble:01x}" * 8
    
    elif logic_type == "alternating":
        # Alternating nibbles (e.g., abababab, 12121212)
        a = (seed + word_index) % 16
        b = (seed * 2 + word_index) % 16
        return f"{a:01x}{b:01x}" * 4
    
    elif logic_type == "mirror":
        # Mirror pattern (e.g., 12344321, abccba)
        half = [(seed + word_index + i) % 16 for i in range(4)]
        full = half + half[::-1]
        return ''.join(f"{x:01x}" for x in full)
    
    elif logic_type == "fibonacci_based":
        # Based on fibonacci sequence
        fib = [1, 1]
        for _ in range(6):
            fib.append((fib[-1] + fib[-2]) % 256)
        start = (seed + word_index) % 4
        return ''.join(f"{fib[(start + i) % len(fib)]:02x}" for i in range(4))
    
    elif logic_type == "prime_based":
        # Based on prime numbers
        primes = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53]
        start = (seed + word_index) % 8
        vals = [primes[(start + i) % len(primes)] for i in range(4)]
        return ''.join(f"{(v * (word_index + 1)) % 256:02x}" for v in vals)
    
    elif logic_type == "palindrome":
        # True palindrome (e.g., 123321, abcba)
        first3 = [(seed + word_index * i) % 16 for i in range(1, 4)]
        middle = (seed + word_index * 4) % 16
        full = first3 + [middle] + first3[::-1]
        return ''.join(f"{x:01x}" for x in full)
    
    elif logic_type == "word_index_based":
        # Based on word's position in official list
        if word and word in OFFICIAL_WORDS:
            idx = OFFICIAL_WORDS.index(word)
            val = (idx * 0xF1234567 + seed) % 0xFFFFFFFF
            return f"{val:08x}"
        return f"{(seed + word_index):08x}"
    
    elif logic_type == "hash_derived":
        # Derived from hash of word + seed
        if word:
            h = hashlib.sha256(f"{word}{seed}".encode()).digest()
            return h[:4].hex()
        return f"{(seed * 0x12345678):08x}"
    
    elif logic_type == "position_shift":
        # Bit shift based on position
        val = ((seed << (word_index % 8)) | (seed >> (8 - word_index % 8))) & 0xFFFFFFFF
        return f"{val:08x}"
    
    elif logic_type == "xor_pattern":
        # XOR-based pattern
        base = seed ^ (word_index * 0x11111111)
        return f"{base:08x}"
    
    elif logic_type == "addition_chain":
        # Addition chain
        start = (seed % 256)
        vals = [start]
        for i in range(3):
            vals.append((vals[-1] + ((seed + word_index) % 256)) % 256)
        return ''.join(f"{v:02x}" for v in vals)
    
    elif logic_type == "multiplication":
        # Multiplicative pattern
        factor = ((seed % 100) + 1)
        mult = ((word_index + 1) * factor * 0x01010101) % 0xFFFFFFFF
        return f"{mult:08x}"
    
    elif logic_type == "exponential":
        # Exponential-like pattern
        base = 2
        exp = (seed + word_index) % 32
        val = pow(base, exp, 0xFFFFFFFF)
        return f"{val:08x}"
    
    elif logic_type == "geometric":
        # Geometric progression
        a = (seed % 256)
        r = ((word_index % 10) + 2)
        vals = [(a * pow(r, i)) % 256 for i in range(4)]
        return ''.join(f"{v:02x}" for v in vals)
    
    elif logic_type == "bitwise_complement":
        # Bitwise complement of sequential
        seq = (seed + word_index) % 0xFFFFFFFF
        comp = (~seq) & 0xFFFFFFFF
        return f"{comp:08x}"
    
    elif logic_type == "rotation":
        # Bit rotation pattern
        val = seed
        rot = (word_index * 4) % 32
        rotated = ((val << rot) | (val >> (32 - rot))) & 0xFFFFFFFF
        return f"{rotated:08x}"
    
    elif logic_type == "shuffle":
        # Shuffled nibbles
        nibbles = [(seed + i) % 16 for i in range(8)]
        # Shuffle based on word_index
        for _ in range(word_index % 7):
            i, j = (seed + _) % 8, (seed + _ * 3) % 8
            nibbles[i], nibbles[j] = nibbles[j], nibbles[i]
        return ''.join(f"{n:01x}" for n in nibbles)
    
    elif logic_type == "interleave":
        # Interleaved values
        a = (seed + word_index) % 0xFFFF
        b = (seed * 2 + word_index) % 0xFFFF
        val = ((a & 0xFF00) | ((b & 0xFF00) >> 8)) << 16 | \
              (((a & 0x00FF) << 8) | (b & 0x00FF))
        return f"{val & 0xFFFFFFFF:08x}"
    
    elif logic_type == "matrix_based":
        # Matrix-based pattern (2x2 matrix flattened)
        m = [
            [(seed + word_index) % 256, (seed * 2 + word_index) % 256],
            [(seed * 3 + word_index) % 256, (seed * 4 + word_index) % 256]
        ]
        return ''.join(f"{m[i][j]:02x}" for i in range(2) for j in range(2))
    
    elif logic_type == "cyclic":
        # Cyclic pattern
        cycle_len = 8
        pos = (seed + word_index) % cycle_len
        pattern = [(seed + i * 31) % 256 for i in range(cycle_len)]
        return ''.join(f"{pattern[(pos + i) % cycle_len]:02x}" for i in range(4))
    
    elif logic_type == "fractal":
        # Fractal-like recursive pattern
        def fractal(n, depth=0):
            if depth >= 3 or n <= 0:
                return n % 256
            return (fractal(n // 2, depth + 1) * 2 + fractal(n // 3, depth + 1)) % 256
        vals = [fractal(seed + word_index + i * 100) for i in range(4)]
        return ''.join(f"{v:02x}" for v in vals)
    
    elif logic_type == "chaos":
        # Chaotic pattern (logistic map-like)
        x = ((seed + word_index) % 1000) / 1000.0
        r = 3.9
        for _ in range(10):
            x = r * x * (1 - x)
        val = int(x * 0xFFFFFFFF) % 0xFFFFFFFF
        return f"{val:08x}"
    
    elif logic_type == "symmetric":
        # Symmetric around center
        left = [(seed + word_index + i) % 16 for i in range(4)]
        right = left[::-1]
        return ''.join(f"{x:01x}" for x in left + right)
    
    else:  # asymmetric
        # Asymmetric unique pattern
        parts = [
            (seed + word_index * 17) % 256,
            (seed * 3 + word_index * 7) % 256,
            (seed * 5 + word_index * 11) % 256,
            (seed * 13 + word_index * 19) % 256
        ]
        return ''.join(f"{p:02x}" for p in parts)

def generate_rule_by_logic(logic_type, seed, word_index):
    """Generate special rule based on logic type"""
    random.seed(seed)
    
    # Determine rule complexity based on logic
    if logic_type in ["sequential_hex", "repeating_nibble"]:
        return "", "", "", "", "", "false"
    
    elif logic_type in ["alternating", "mirror"]:
        fixed1 = f"{(seed + word_index) % 16:01x}"
        fixed2 = f"{(seed * 2 + word_index) % 16:01x}"
        return "", fixed1, fixed2, "", "", "false"
    
    elif logic_type in ["fibonacci_based", "prime_based"]:
        pad = f"{(seed % 16):01x}"
        return "", "", "", "", pad, "false"
    
    elif logic_type == "palindrome":
        fixed8 = generate_hex_by_logic(logic_type, seed, word_index)
        return fixed8, "", "", "", "", "true"
    
    elif logic_type in ["hash_derived", "word_index_based"]:
        fixed3 = f"{(seed + word_index) % 256:02x}"
        return "", "", "", fixed3, "", "false"
    
    elif logic_type == "xor_pattern":
        fixed1 = f"{(seed % 16):01x}"
        fixed2 = f"{((seed >> 4) % 16):01x}"
        return "", fixed1, fixed2, "", "", "true"
    
    else:
        # Complex rules for advanced logic types
        fixed1 = f"{(seed % 16):01x}"
        fixed2 = f"{((seed + word_index) % 16):01x}"
        fixed3 = f"{((seed * 2) % 16):01x}"
        pad = f"{(word_index % 16):01x}"
        repeat = "true" if (seed + word_index) % 3 == 0 else "false"
        return "", fixed1, fixed2, fixed3, pad, repeat

def generate_profile_content(profile_num, total=500):
    """Generate content for a single profile using official wordlist"""
    
    # Generate unique name
    adj = ADJECTIVES[profile_num % len(ADJECTIVES)]
    noun = NOUNS[(profile_num * 7) % len(NOUNS)]
    version = f"v{(profile_num % 10)}.{(profile_num % 100)//10}"
    name = f"{adj}_{noun}_{version}"
    
    # Select logic type (cycle through all 25 types)
    logic_type = LOGIC_TYPES[profile_num % len(LOGIC_TYPES)]
    
    # Determine profile characteristics
    profile_seed = profile_num * 7919  # Prime number for better distribution
    random.seed(profile_seed)
    
    checksum_types = ["v1", "v2", "none"]
    checksum = checksum_types[profile_num % 3]
    strict = (profile_num % 2) == 0
    
    # Number of words (8-24 from official list)
    num_words = 8 + (profile_num % 17)
    
    # Number of rules (3-15)
    num_rules = 3 + (profile_num % 13)
    
    lines = []
    lines.append(f"# AKM Profile - {name}")
    lines.append(f"# Auto-generated profile #{profile_num + 1} of {total}")
    lines.append(f"# Logic Type: {logic_type}")
    lines.append(f"# Wordlist: wordlist_512_ascii.txt (512 official words)")
    
    if strict:
        lines.append("#STRICT_HEX")
    
    lines.append("")
    lines.append("#METADATA")
    lines.append(f"name={name}")
    lines.append(f"description=Profile using {logic_type} logic with {num_words} word mappings from official 512-word list")
    lines.append(f"author=ProfileGenerator v3.0")
    lines.append(f"version=1.0.{profile_num}")
    lines.append(f"created={datetime.now().strftime('%Y-%m-%d')}")
    lines.append(f"checksum={checksum}")
    lines.append(f"strict={'true' if strict else 'false'}")
    lines.append(f"algorithm={logic_type}")
    lines.append(f"wordlist=wordlist_512_ascii.txt")
    
    lines.append("")
    lines.append(f"#WORDS")
    lines.append(f"# Custom hex mappings using '{logic_type}' logic pattern")
    
    # Select words deterministically from official list
    used_words = set()
    word_mappings = []
    
    for i in range(num_words):
        # Select word from official 512 list
        word_idx = (profile_num * 100 + i * 17) % 512
        word = OFFICIAL_WORDS[word_idx]
        
        if word in used_words:
            continue
        used_words.add(word)
        
        # Generate hex based on logic type
        hex_val = generate_hex_by_logic(logic_type, profile_seed, i, word)
        word_mappings.append((word, hex_val))
        lines.append(f"{word}={hex_val}")
    
    lines.append("")
    lines.append("#RULES")
    lines.append("# Special processing rules")
    lines.append("# Format: word|fixed8|fixed1|fixed2|fixed3|pad_nibble|repeat")
    
    # Generate rules using different words
    used_rule_words = set()
    for i in range(num_rules):
        word_idx = (profile_num * 50 + i * 31) % 512
        word = OFFICIAL_WORDS[word_idx]
        
        if word in used_rule_words or word in used_words:
            continue
        used_rule_words.add(word)
        
        # Generate rule based on logic type
        fixed8, fixed1, fixed2, fixed3, pad, repeat = generate_rule_by_logic(
            logic_type, profile_seed + 1000, i
        )
        
        lines.append(f"{word}|{fixed8}|{fixed1}|{fixed2}|{fixed3}|{pad}|{repeat}")
    
    lines.append("")
    lines.append("# End of generated profile")
    lines.append(f"# Total words mapped: {len(word_mappings)}")
    lines.append(f"# Total rules: {len(used_rule_words)}")
    
    return name, '\n'.join(lines)

def main():
    print(f"Generating 500 AKM profiles in {OUTPUT_DIR}/")
    print(f"Using official wordlist: {WORDLIST_FILE} ({len(OFFICIAL_WORDS)} words)")
    print()
    
    profile_list = []
    
    for i in range(500):
        name, content = generate_profile_content(i, 500)
        filename = f"profile_{i+1:03d}_{name}.txt"
        filepath = os.path.join(OUTPUT_DIR, filename)
        
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        
        profile_list.append((i+1, name, filename))
        
        if (i + 1) % 50 == 0:
            print(f"  Generated {i + 1}/500 profiles...")
    
    # Generate detailed index file
    index_path = os.path.join(OUTPUT_DIR, "_index.txt")
    with open(index_path, 'w', encoding='utf-8') as f:
        f.write("# AKM Profile Index - 500 Profiles\n")
        f.write(f"# Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"# Wordlist: {WORDLIST_FILE}\n")
        f.write(f"# Total profiles: 500\n")
        f.write(f"# Logic types: {', '.join(LOGIC_TYPES)}\n\n")
        f.write("# ID | Name | Logic Type | Filename\n")
        f.write("-" * 80 + "\n")
        for pid, name, filename in profile_list:
            logic = LOGIC_TYPES[(pid - 1) % len(LOGIC_TYPES)]
            f.write(f"{pid:3d} | {name:40s} | {logic:20s} | {filename}\n")
    
    # Generate summary file
    summary_path = os.path.join(OUTPUT_DIR, "_summary.txt")
    with open(summary_path, 'w', encoding='utf-8') as f:
        f.write("=" * 60 + "\n")
        f.write("AKM Profile Generation Summary\n")
        f.write("=" * 60 + "\n\n")
        f.write(f"Total profiles generated: 500\n")
        f.write(f"Wordlist used: {WORDLIST_FILE}\n")
        f.write(f"Words in wordlist: {len(OFFICIAL_WORDS)}\n")
        f.write(f"Logic types used: {len(LOGIC_TYPES)}\n\n")
        
        f.write("Logic Type Distribution:\n")
        f.write("-" * 40 + "\n")
        for i, logic in enumerate(LOGIC_TYPES):
            count = (500 // len(LOGIC_TYPES)) + (1 if i < (500 % len(LOGIC_TYPES)) else 0)
            f.write(f"  {logic:20s}: {count} profiles\n")
        
        f.write("\nChecksum Version Distribution:\n")
        f.write("-" * 40 + "\n")
        f.write(f"  V1 (9-bit):  ~{500//3} profiles\n")
        f.write(f"  V2 (10-bit): ~{500//3} profiles\n")
        f.write(f"  None:        ~{500//3} profiles\n")
        
        f.write("\nStrict Mode Distribution:\n")
        f.write("-" * 40 + "\n")
        f.write(f"  Strict:      ~{500//2} profiles\n")
        f.write(f"  Non-strict:  ~{500//2} profiles\n")
    
    print()
    print(f"Done! Generated 500 profiles in {OUTPUT_DIR}/")
    print(f"Index file: {index_path}")
    print(f"Summary file: {summary_path}")
    print()
    print("Sample profiles:")
    for i in range(min(5, len(profile_list))):
        pid, name, filename = profile_list[i]
        print(f"  {pid}. {name} -> {filename}")

if __name__ == "__main__":
    main()
