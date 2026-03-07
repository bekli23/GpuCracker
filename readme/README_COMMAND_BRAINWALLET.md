# BRAINWALLET Mode - 50+ Command Examples

## Basic Recovery (1-10)

### 1. Basic wordlist attack
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt
```
SHA256(password) brainwallet.

### 2. Warp wallet
```bash
./GpuCracker --mode brainwallet --type warp --wordlist passwords.txt
```

### 3. With target address
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --target-address 1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa
```

### 4. GPU acceleration
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --gpu-type cuda
```

### 5. Multi-GPU
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --gpu-type cuda --device-id 0,1
```

### 6. With salt
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --salt "mysalt"
```

### 7. Pattern-based
```bash
./GpuCracker --mode brainwallet --pattern "Password{0000-9999}"
```

### 8. Combination attack
```bash
./GpuCracker --mode brainwallet --wordlist words.txt --rules rules.txt
```

### 9. Mask attack
```bash
./GpuCracker --mode brainwallet --mask "?l?l?l?l?d?d?d?d"
```

### 10. Hybrid attack
```bash
./GpuCracker --mode brainwallet --wordlist words.txt --append-numbers 0-9999
```

## Advanced Attacks (11-30)

### 11. Large wordlist
```bash
./GpuCracker --mode brainwallet --wordlist rockyou.txt --batch-size 10000
```

### 12. Multiple wordlists
```bash
./GpuCracker --mode brainwallet --wordlist "list1.txt,list2.txt,list3.txt"
```

### 13. Brainv2 format
```bash
./GpuCracker --mode brainwallet --type brainv2 --wordlist passwords.txt
```

### 14. Bitaddress format
```bash
./GpuCracker --mode brainwallet --type bitaddress --wordlist passwords.txt
```

### 15. Custom KDF
```bash
./GpuCracker --mode brainwallet --custom-kdf ./kdf.so --wordlist passwords.txt
```

### 16. Leetspeak variants
```bash
./GpuCracker --mode brainwallet --wordlist words.txt --leet-speak
```

### 17. Case mutations
```bash
./GpuCracker --mode brainwallet --wordlist words.txt --case-mutations
```

### 18. Prefix/suffix append
```bash
./GpuCracker --mode brainwallet --wordlist words.txt --prefixes "123,!,$" --suffixes "123,!,$"
```

### 19. Character substitution
```bash
./GpuCracker --mode brainwallet --wordlist words.txt --substitutions "a:@,s:$,i:1"
```

### 20. Keyboard walk
```bash
./GpuCracker --mode brainwallet --keyboard-walk
```

### 21. Common passwords
```bash
./GpuCracker --mode brainwallet --common-passwords --years 2010-2024
```

### 22. Passphrases
```bash
./GpuCracker --mode brainwallet --passphrases phrases.txt
```

### 23. Sentence mode
```bash
./GpuCracker --mode brainwallet --sentences sentences.txt
```

### 24. Poetry mode
```bash
./GpuCracker --mode brainwallet --poetry --wordlist words.txt --lines 4
```

### 25. Bible phrases
```bash
./GpuCracker --mode brainwallet --bible --translation KJV
```

### 26. Famous quotes
```bash
./GpuCracker --mode brainwallet --quotes --category famous
```

### 27. Movie quotes
```bash
./GpuCracker --mode brainwallet --quotes --category movies
```

### 28. Song lyrics
```bash
./GpuCracker --mode brainwallet --lyrics --artist "Beatles"
```

### 29. Book phrases
```bash
./GpuCracker --mode brainwallet --books --title "1984"
```

### 30. Wikipedia titles
```bash
./GpuCracker --mode brainwallet --wikipedia --category "Cryptography"
```

## Targeted Recovery (31-50)

### 31. With public key
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --target-pubkey 0279BE...
```

### 32. With multiple targets
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --targets targets.txt
```

### 33. With bloom filter
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --bloom-keys addresses.blf
```

### 34. Progress save
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --checkpoint brain.chk
```

### 35. Resume scan
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --resume brain.chk
```

### 36. Speed limit
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --speed-limit 1000000
```

### 37. Time limit
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --time-limit 3600
```

### 38. Probability sort
```bash
./GpuCracker --mode brainwallet --wordlist passwords.txt --sort-probability
```

### 39. Markov chains
```bash
./GpuCracker --mode brainwallet --markov --training passwords.txt
```

### 40. PRINCE algorithm
```bash
./GpuCracker --mode brainwallet --prince --wordlist elements.txt
```

### 41. OMEN algorithm
```bash
./GpuCracker --mode brainwallet --omen --level 4
```

### 42. PCFG
```bash
./GpuCracker --mode brainwallet --pcfg --training passwords.txt
```

### 43. Known password parts
```bash
./GpuCracker --mode brainwallet --known-prefix "My" --known-suffix "2024" --mask "?l?l?l"
```

### 44. Date combinations
```bash
./GpuCracker --mode brainwallet --dates 1970-2024 --format MMDDYYYY
```

### 45. Phone numbers
```bash
./GpuCracker --mode brainwallet --phone-numbers --country US
```

### 46. License plates
```bash
./GpuCracker --mode brainwallet --license-plates --region CA
```

### 47. Names variations
```bash
./GpuCracker --mode brainwallet --names names.txt --variations full
```

### 48. Company names
```bash
./GpuCracker --mode brainwallet --companies companies.txt --year-founded
```

### 49. Multi-language
```bash
./GpuCracker --mode brainwallet --wordlist words.txt --languages en,es,fr,de
```

### 50. Full research mode
```bash
./GpuCracker --mode brainwallet \
  --wordlist rockyou.txt \
  --type warp \
  --rules rules_comprehensive.txt \
  --leet-speak \
  --case-mutations \
  --keyboard-walk \
  --common-passwords \
  --dates 2009-2024 \
  --bloom-keys btc_used.blf \
  --gpu-type cuda \
  --device-id 0,1,2,3 \
  --checkpoint brain_research.chk
```

## Warning

Brain wallets are **cryptographically broken**. Never use them for actual funds.

## Statistics

| Attack Type | Speed (GPU) | Effectiveness |
|-------------|-------------|---------------|
| SHA256 | 10 GH/s | Very High |
| Warp | 100 MH/s | High |
| PBKDF2 | 1 MH/s | Medium |
| Scrypt | 100 KH/s | Low |
