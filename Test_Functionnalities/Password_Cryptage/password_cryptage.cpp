#include <iostream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>

#define AES_KEY_SIZE 32   // 256-bit key
#define AES_IV_SIZE 16    // Initialization Vector

void handleErrors()
{
    std::cerr << "Error occurred!" << std::endl;
    exit(1);
}

// Encrypt function
std::string encryptAES(const std::string& plaintext, unsigned char* key, unsigned char* iv)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);

    std::string ciphertext(plaintext.size() + AES_IV_SIZE, '\0');
    int len;
    EVP_EncryptUpdate(ctx, (unsigned char*)&ciphertext[0], &len, (unsigned char*)plaintext.c_str(), plaintext.size());
    int final_len;
    EVP_EncryptFinal_ex(ctx, (unsigned char*)&ciphertext[len], &final_len);
    EVP_CIPHER_CTX_free(ctx);
    
    ciphertext.resize(len + final_len);
    return ciphertext;
}

// Decrypt function
std::string decryptAES(const std::string& ciphertext, unsigned char* key, unsigned char* iv)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);

    std::string plaintext(ciphertext.size(), '\0');
    int len;
    EVP_DecryptUpdate(ctx, (unsigned char*)&plaintext[0], &len, (unsigned char*)ciphertext.c_str(), ciphertext.size());
    int final_len;
    EVP_DecryptFinal_ex(ctx, (unsigned char*)&plaintext[len], &final_len);
    EVP_CIPHER_CTX_free(ctx);

    plaintext.resize(len + final_len);
    return plaintext;
}

int main()
{
    std::string password;
    std::cout << "Enter password to encrypt: ";
    std::cin >> password;

    // Generate a random key and IV
    unsigned char key[AES_KEY_SIZE], iv[AES_IV_SIZE];
    RAND_bytes(key, AES_KEY_SIZE);
    RAND_bytes(iv, AES_IV_SIZE);

    // Encrypt
    std::string encrypted = encryptAES(password, key, iv);
    std::cout << "Encrypted Password: " << encrypted << std::endl;

    // Decrypt
    std::string decrypted = decryptAES(encrypted, key, iv);
    std::cout << "Decrypted Password: " << decrypted << std::endl;

    return 0;
}
