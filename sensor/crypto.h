String sign_message(const char* privateKey, const char* message) {
    // Initialize mbedTLS structures
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    unsigned char hash[32];       // SHA-256 hash output (32 bytes)
    unsigned char signature[256]; // Buffer for RSA signature (256 bytes for up to 2048-bit keys)
    const char *pers = "rsa_sign";
    String sigHex = "";
    size_t sig_len;               // Variable to hold the signature length
    String hashHex = "";

    // Initialize contexts
    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Seed the RNG
    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                              (const unsigned char *)pers, strlen(pers)) != 0) {
        Serial.println("Failed to seed RNG");
        goto cleanup;
    }

    // Parse the private key
    if (mbedtls_pk_parse_key(&pk, (const unsigned char *)privateKey,
                             strlen(privateKey) + 1, NULL, 0,
                             mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
        Serial.println("Failed to parse private key");
        goto cleanup;
    }

    // Compute SHA-256 hash of the message
    if (mbedtls_sha256((const unsigned char *)message, strlen(message), hash, 0) != 0) {
        Serial.println("Failed to compute SHA-256 hash");
        goto cleanup;
    }


    // Sign the hash
    if (mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, 32, signature, sizeof(signature), &sig_len,
                        mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
        Serial.println("Failed to sign the message");
        goto cleanup;
    }


    // Convert signature to hex string
    for (size_t i = 0; i < sig_len ; i++) {
        char hex[3];
        sprintf(hex, "%02x", signature[i]);
        sigHex += hex;
    }

    // Convert hash to hex string
    for (size_t i = 0; i < 32 ; i++) {
        char hex[3];
        sprintf(hex, "%02x", hash[i]);
        hashHex += hex;
    }


    Serial.print("Hash: ");
    Serial.println(hashHex);

cleanup:
    // Free resources
    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);

    return sigHex;
}