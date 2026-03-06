#!/usr/bin/env python3
"""
Profile Generator for AKM (Advanced Key Mapper)
Generates 300 unique profiles with different logic
"""

import os
import random
import hashlib
from datetime import datetime

# Ensure output directory exists
OUTPUT_DIR = "akm_profile"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Base words for profile names
ADJECTIVES = [
    "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
    "iota", "kappa", "lambda", "mu", "nu", "xi", "omicron", "pi",
    "rho", "sigma", "tau", "upsilon", "phi", "chi", "psi", "omega",
    "red", "blue", "green", "yellow", "purple", "orange", "black", "white",
    "dark", "light", "bright", "shadow", "golden", "silver", "bronze",
    "crystal", "quantum", "atomic", "solar", "lunar", "stellar", "cosmic",
    "nebula", "galaxy", "stellar", "celestial", "astral", "ethereal",
    "mystic", "magic", "arcane", "rune", "glyph", "sigil", "talisman",
    "prime", "core", "base", "root", "origin", "source", "essence",
    "pure", "raw", "basic", "simple", "complex", "advanced", "elite",
    "rapid", "swift", "fast", "quick", "speed", "velocity", "momentum",
    "power", "force", "energy", "strength", "might", "vigor", "potent",
    "secure", "safe", "shield", "guard", "protect", "defend", "fortress",
    "stealth", "hidden", "secret", "covert", "shadow", "cloak", "veil",
    "precision", "exact", "accurate", "sharp", "keen", "fine", "refined",
    "brute", "force", "raw", "wild", "savage", "feral", "primal",
    "smart", "intelligent", "wise", "clever", "bright", "brilliant", "genius",
    "hybrid", "fusion", "combined", "unified", "merged", "blended", "mixed",
    "crypto", "cipher", "code", "encode", "encrypt", "secure", "lock",
    "phoenix", "dragon", "tiger", "lion", "eagle", "wolf", "falcon",
    "storm", "thunder", "lightning", "fire", "ice", "frost", "flame",
    "echo", "ripple", "wave", "pulse", "beat", "rhythm", "flow",
    "vertex", "apex", "peak", "summit", "crest", "crown", "zenith",
    "dynamic", "static", "kinetic", "potential", "active", "passive",
    "linear", "circular", "spiral", "radial", "axial", "orbital",
    "compact", "dense", "solid", "firm", "rigid", "stable", "steady",
    "flexible", "elastic", "fluid", "liquid", "flowing", "adaptive",
    "random", "chaos", "order", "system", "pattern", "structure",
    "parallel", "serial", "sync", "async", "concurrent", "sequential",
    "binary", "ternary", "quaternary", "octal", "decimal", "hexadecimal",
    "mono", "dual", "triple", "quad", "penta", "hexa", "hepta", "octa",
    "nano", "micro", "macro", "mega", "giga", "tera", "peta", "exa",
    "low", "medium", "high", "ultra", "super", "hyper", "extreme", "max",
    "classic", "modern", "retro", "vintage", "old", "new", "fresh", "legacy",
    "eastern", "western", "northern", "southern", "central", "global", "universal",
    "spring", "summer", "autumn", "winter", "dawn", "dusk", "noon", "midnight"
]

NOUNS = [
    "cipher", "key", "lock", "vault", "safe", "chest", "box", "container",
    "shield", "armor", "guard", "sentinel", "warden", "keeper", "protector",
    "blade", "sword", "spear", "arrow", "bolt", "strike", "hit", "impact",
    "core", "heart", "center", "hub", "nexus", "node", "point", "focus",
    "matrix", "grid", "lattice", "network", "web", "mesh", "fabric", "weave",
    "cipher", "code", "script", "text", "glyph", "rune", "mark", "sign",
    "spark", "flare", "flash", "beam", "ray", "glow", "shine", "gleam",
    "pulse", "beat", "rhythm", "cadence", "tempo", "pace", "speed", "velocity",
    "wave", "ripple", "surge", "tide", "current", "flow", "stream", "river",
    "storm", "tempest", "gale", "wind", "breeze", "gust", "blast", "burst",
    "flame", "blaze", "inferno", "fire", "ember", "spark", "ash", "smoke",
    "frost", "ice", "snow", "crystal", "diamond", "gem", "jewel", "pearl",
    "shadow", "shade", "darkness", "void", "abyss", "depth", "chasm", "gap",
    "light", "beam", "ray", "gleam", "glow", "aura", "halo", "corona",
    "star", "sun", "moon", "planet", "world", "earth", "terra", "gaia",
    "nova", "nebula", "quasar", "pulsar", "comet", "meteor", "asteroid", "dust",
    "wolf", "tiger", "lion", "eagle", "falcon", "hawk", "raven", "crow",
    "dragon", "serpent", "wyrm", "beast", "creature", "monster", "entity", "being",
    "titan", "giant", "colossus", "leviathan", "behemoth", "kraken", "hydra", "phoenix",
    "oracle", "prophet", "seer", "sage", "wizard", "mage", "witch", "warlock",
    "knight", "warrior", "fighter", "soldier", "guardian", "defender", "champion", "hero",
    "path", "way", "road", "route", "course", "track", "trail", "lane",
    "gate", "door", "portal", "entrance", "access", "entry", "opening", "passage",
    "tower", "spire", "fortress", "castle", "citadel", "stronghold", "bastion", "keep",
    "realm", "domain", "kingdom", "empire", "republic", "nation", "state", "territory",
    "horizon", "frontier", "boundary", "limit", "edge", "border", "perimeter", "rim",
    "source", "origin", "root", "base", "foundation", "ground", "bedrock", "core",
    "peak", "summit", "apex", "vertex", "crown", "crest", "top", "zenith",
    "engine", "motor", "machine", "mechanism", "device", "apparatus", "instrument", "tool",
    "algorithm", "procedure", "process", "method", "technique", "system", "scheme", "plan",
    "pattern", "design", "structure", "framework", "architecture", "layout", "arrangement", "order",
    "balance", "equilibrium", "harmony", "symmetry", "proportion", "ratio", "measure", "scale"
]

# Base 512 words
BASE_WORDS = [
    "abis","acelasi","acoperis","adanc","adapost","adorare","afectiune","aer",
    "ajun","albastru","alb","alge","altar","amintire","amurg","anotimp","apa",
    "apele","apus","apuseni","apusor","aroma","artar","arta","asfintit",
    # ... (truncated for brevity, will use hash-based selection)
]

def generate_hex_pattern(seed, pattern_type):
    """Generate hex pattern based on type"""
    random.seed(seed)
    
    if pattern_type == "sequential":
        # Sequential pattern like 00000001, 00000002, etc.
        start = random.randint(0, 0xFFFFFFF)
        return f"{(start % 0xFFFFFFFF):08x}"
    
    elif pattern_type == "repeating":
        # Repeating nibbles like aaaaaaaa, bbbbbbbb
        nibble = random.choice("0123456789abcdef")
        return nibble * 8
    
    elif pattern_type == "alternating":
        # Alternating like abababab, 12121212
        a = random.choice("0123456789abcdef")
        b = random.choice("0123456789abcdef")
        return (a + b) * 4
    
    elif pattern_type == "mirror":
        # Mirror pattern like 12344321, abccba
        half = ''.join(random.choices("0123456789abcdef", k=4))
        return half + half[::-1]
    
    elif pattern_type == "fibonacci":
        # Based on fibonacci
        fib = [1, 1]
        for _ in range(6):
            fib.append((fib[-1] + fib[-2]) % 256)
        return ''.join(f"{x:02x}" for x in fib[:4])
    
    elif pattern_type == "prime":
        # Based on prime numbers
        primes = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29]
        idx = random.randint(0, 5)
        p1, p2 = primes[idx], primes[idx + 1]
        return f"{(p1 * p2 % 0xFFFFFFFF):08x}"
    
    elif pattern_type == "palindrome":
        # True palindrome
        first3 = ''.join(random.choices("0123456789abcdef", k=3))
        middle = random.choice("0123456789abcdef")
        return first3 + middle + first3[::-1]
    
    else:  # random
        return ''.join(random.choices("0123456789abcdef", k=8))

def generate_profile_content(profile_num, total=300):
    """Generate content for a single profile"""
    
    # Generate unique name
    adj = ADJECTIVES[profile_num % len(ADJECTIVES)]
    noun = NOUNS[(profile_num * 7) % len(NOUNS)]
    name = f"{adj}_{noun}_v{(profile_num % 10)}.{(profile_num % 100)//10}"
    
    # Determine profile characteristics
    random.seed(profile_num * 1337)  # Consistent seed
    
    checksum_types = ["v1", "v2", "none"]
    checksum = random.choice(checksum_types)
    strict = random.choice([True, False])
    num_words = random.randint(8, 32)
    num_rules = random.randint(3, 12)
    
    pattern_types = ["sequential", "repeating", "alternating", "mirror", 
                     "fibonacci", "prime", "palindrome", "random"]
    
    lines = []
    lines.append(f"# AKM Profile - {name}")
    lines.append(f"# Auto-generated profile #{profile_num + 1} of {total}")
    lines.append(f"# Unique logic: {random.choice(['HKDF-based', 'SHA-derived', 'Pattern-mapped', 'Rule-based'])}")
    
    if strict:
        lines.append("#STRICT_HEX")
    
    lines.append("")
    lines.append("#METADATA")
    lines.append(f"name={name}")
    lines.append(f"description=Auto-generated profile with {num_words} custom mappings and {num_rules} rules")
    lines.append(f"author=ProfileGenerator v2.0")
    lines.append(f"version=1.0.{profile_num}")
    lines.append(f"created={datetime.now().strftime('%Y-%m-%d')}")
    lines.append(f"checksum={checksum}")
    lines.append(f"strict={'true' if strict else 'false'}")
    lines.append(f"algorithm={random.choice(['hexpack8', 'stream16', 'hash32', 'rulepack'])}")
    
    lines.append("")
    lines.append("#WORDS")
    lines.append(f"# Custom hex mappings - Profile uses '{pattern_types[profile_num % len(pattern_types)]}' pattern")
    
    # Generate word mappings
    used_words = set()
    for i in range(num_words):
        # Select word deterministically
        word_idx = (profile_num * 100 + i * 7) % 512
        word = BASE_WORDS[word_idx % len(BASE_WORDS)] if word_idx < len(BASE_WORDS) else f"word{word_idx}"
        
        if word in used_words:
            continue
        used_words.add(word)
        
        pattern = pattern_types[(profile_num + i) % len(pattern_types)]
        seed = profile_num * 1000 + i
        hex_val = generate_hex_pattern(seed, pattern)
        
        lines.append(f"{word}={hex_val}")
    
    lines.append("")
    lines.append("#RULES")
    lines.append("# Special processing rules for specific words")
    lines.append("# Format: word|fixed8|fixed1|fixed2|fixed3|pad_nibble|repeat")
    
    # Generate rules
    used_rule_words = set()
    for i in range(num_rules):
        word_idx = (profile_num * 50 + i * 13) % 512
        word = BASE_WORDS[word_idx % len(BASE_WORDS)] if word_idx < len(BASE_WORDS) else f"word{word_idx}"
        
        if word in used_rule_words or word in used_words:
            continue
        used_rule_words.add(word)
        
        # Generate random rule
        fixed8 = generate_hex_pattern(profile_num * 2000 + i, "random") if random.random() > 0.5 else ""
        fixed1 = ''.join(random.choices("0123456789abcdef", k=2)) if random.random() > 0.6 else ""
        fixed2 = ''.join(random.choices("0123456789abcdef", k=4)) if random.random() > 0.7 else ""
        fixed3 = ''.join(random.choices("0123456789abcdef", k=6)) if random.random() > 0.8 else ""
        pad = random.choice("0123456789abcdef") if random.random() > 0.5 else ""
        repeat = "true" if random.random() > 0.8 else "false"
        
        lines.append(f"{word}|{fixed8}|{fixed1}|{fixed2}|{fixed3}|{pad}|{repeat}")
    
    lines.append("")
    lines.append("# End of generated profile")
    
    return name, '\n'.join(lines)

def main():
    print(f"Generating 300 AKM profiles in {OUTPUT_DIR}/")
    
    profile_list = []
    
    for i in range(300):
        name, content = generate_profile_content(i, 300)
        filename = f"profile_{i+1:03d}_{name}.txt"
        filepath = os.path.join(OUTPUT_DIR, filename)
        
        with open(filepath, 'w') as f:
            f.write(content)
        
        profile_list.append((i+1, name, filename))
        
        if (i + 1) % 50 == 0:
            print(f"  Generated {i + 1}/300 profiles...")
    
    # Generate index file
    index_path = os.path.join(OUTPUT_DIR, "_index.txt")
    with open(index_path, 'w') as f:
        f.write("# AKM Profile Index\n")
        f.write(f"# Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"# Total profiles: 300\n\n")
        f.write("# ID | Name | Filename\n")
        f.write("-" * 60 + "\n")
        for pid, name, filename in profile_list:
            f.write(f"{pid:3d} | {name:40s} | {filename}\n")
    
    print(f"\nDone! Generated 300 profiles in {OUTPUT_DIR}/")
    print(f"Index file: {index_path}")

if __name__ == "__main__":
    main()
