# 🔒 AES-256 Encryption & Decryption (C++ with OpenSSL)

This program demonstrates how to **encrypt and decrypt text using AES-256-CBC** with the OpenSSL library in C++.  
It uses randomly generated keys and initialization vectors (IVs) for each run, that must be stored (e.g. in a secured database) to decrypt later.

---

## 🚀 Features

- AES-256-CBC symmetric encryption  
- Secure random key and IV generation with OpenSSL’s `RAND_bytes()`  
- Proper use of OpenSSL EVP API (`EVP_EncryptInit_ex`, `EVP_EncryptUpdate`, `EVP_DecryptFinal_ex`)  
- Clean and minimal implementation for learning or integration purposes  

---

## 🧠 How It Works

1. Prompts the user to enter a password (plaintext).  
2. Generates:
   - a **256-bit AES key** (32 bytes)
   - a **128-bit IV** (16 bytes)  
3. Encrypts the plaintext using AES-256-CBC.  
4. Decrypts the ciphertext back to plaintext to verify correctness.  

---

## 🧩 Code Structure

| Function | Description |
|-----------|--------------|
| `encryptAES()` | Encrypts plaintext with AES-256-CBC using provided key and IV |
| `decryptAES()` | Decrypts ciphertext back to plaintext |
| `handleErrors()` | Handles encryption/decryption errors |
| `main()` | Runs encryption + decryption demo with user input |

---

## 🛠️ Compilation

Make sure you have **OpenSSL** installed.

```bash
g++ -std=c++17 -Wall -Wfatal-errors password_cryptage.cpp -o password_cryptage.x -I/opt/homebrew/opt/openssl@3/include -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto
```

---

## ▶️ Usage

```bash
./password_cryptage.x
```

Example
```yaml
Enter password to encrypt: Secret123    
Encrypted Password: g�M޶Ĥ�9�n
Decrypted Password: Secret123
```
