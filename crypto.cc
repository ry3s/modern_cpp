#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cryptlib.h"
#include "default.h"
#include "files.h"
#include "filters.h"
#include "hex.h"
#include "osrng.h"
#include "queue.h"
#include "rsa.h"
#include "sha.h"
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "md5.h"

std::string caesar_encrypt(std::string_view text, int shift) {
  std::string result = "";
  for (const char ch : text) {
    if (isalpha(ch) && isupper(ch)) {
      result += 'A' + (ch - 'A' + shift) % 26;
    } else {
      result += ch;
    }
  }
  return result;
}

std::string caesar_decrypt(std::string_view text, int shift) {
  return caesar_encrypt(text, 26 - shift);
}

void test_caesar() {
  std::string text = "HELLO WORLD";
  int shift = 3;
  auto encrypted = caesar_encrypt(text, shift);
  auto decrypted = caesar_decrypt(encrypted, shift);
  std::cout << "ENC: " << encrypted << std::endl;
  ;
  std::cout << "DEC: " << decrypted << std::endl;

  assert(text == decrypted);
}

std::string make_vigenere_table() {
  std::string table;
  for (int i = 0; i < 26; i++) {
    table += caesar_encrypt("ABCDEFGHIJKLMNOPQRSTUVWXYZ", i);
  }
  return table;
}

std::string vigenere_encrypt(std::string_view text, std::string_view key) {
  std::string result;
  const auto table = make_vigenere_table();

  for (int i = 0; i < text.size(); i++) {
    auto row = key[i % key.size()] - 'A';
    auto col = text[i] - 'A';
    result += table[row * 26 + col];
  }

  return result;
}

std::string vigenere_decrypt(std::string_view text, std::string_view key) {
  std::string result;
  const auto table = make_vigenere_table();

  for (int i = 0; i < text.size(); i++) {
    auto row = key[i % key.size()] - 'A';

    for (int col = 0; col < 26; col++) {
      if (table[row * 26 + col] == text[i]) {
        result += 'A' + col;
        break;
      }
    }
  }

  return result;
}

void test_vigenere() {
  auto text = "THECPPCHALLENGE";
  auto encrypted = vigenere_encrypt(text, "SAMPLE");
  auto decrypted = vigenere_decrypt(encrypted, "SAMPLE");
  std::cout << "ENC: " << encrypted << std::endl;
  ;
  std::cout << "DEC: " << decrypted << std::endl;

  assert(text == decrypted);
}

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(std::vector<unsigned char> bytes) {
  const char padding = '=';
  std::string result;

  int i = 0;
  for (; i < bytes.size(); i += 3) {
    unsigned value = (bytes[i] << 16) | (bytes[i + 1] << 8) | bytes[i + 2];

    result += base64_chars[(value >> 18) & 0x3f];
    result += base64_chars[(value >> 12) & 0x3f];
    result += base64_chars[(value >> 6) & 0x3f];
    result += base64_chars[value & 0x3f];
  }
  std::cout << result << std::endl;
  std::cout << i << std::endl;
  int rest = bytes.size() - i;
  if (rest == 1) {
    unsigned value = bytes[i];
    result += base64_chars[(value >> 2) & 0x3f];
    result += base64_chars[(value & 0x03) << 4];
    result += padding;
    result += padding;
  } else if (rest == 2) {
    unsigned value = (bytes[i] << 8) | bytes[i + 1];
    result += base64_chars[(value >> 10) & 0x3f];
    result += base64_chars[(value >> 4) & 0x3f];
    result += base64_chars[(value & 0x0f) << 2];
    result += padding;
  }

  return result;
}

std::vector<unsigned char> base64_decode(std::string data) {
  const unsigned char padding = '=';
  auto valid_base64_char = [](unsigned char ch) {
    char c = base64_chars[ch];
    return isalnum(c) || (c == '+') || (c == '/') || (c == '=');
  };

  std::vector<unsigned char> result;
  int i = 0;
  for (; i < data.size(); i += 4) {
    unsigned char c1 = base64_chars.find(data[i]);
    unsigned char c2 = base64_chars.find(data[i + 1]);
    unsigned char c3 = base64_chars.find(data[i + 2]);
    unsigned char c4 = base64_chars.find(data[i + 3]);
    if (!valid_base64_char(c1) || !valid_base64_char(c2) || !valid_base64_char(c3) ||
        !valid_base64_char(c4)) {
      throw std::runtime_error("invalid base64 encoding");
    }

    if (c3 == padding && c4 == padding) {
      unsigned value = (c1 << 6) | c2;
      result.push_back((value >> 4) & 0xff);
    } else if (c4 == padding) {
      unsigned value = (c1 << 12) | (c2 << 6) | c3;
      result.push_back((value >> 10) & 0xff);
      result.push_back((value >> 2) & 0xff);
    } else {
      unsigned value = (c1 << 18) | (c2 << 12) | (c3 << 6) | c4;
      result.push_back((value >> 16) & 0xff);
      result.push_back((value >> 8) & 0xff);
      result.push_back(value & 0xff);
    }
  }

  return result;
}

void test_base64() {
  std::string str = R"(sample)";
  std::vector<unsigned char> data(str.begin(), str.end());
  auto enc = base64_encode(data);
  std::cout << "ENC: " << enc << std::endl;
  auto dec = base64_decode(enc);
  std::cout << "DEC: " << std::string(dec.begin(), dec.end()) << std::endl;
}

struct User {
  int id;
  std::string username;
  std::string password;
  std::string firstname;
  std::string lastname;
};

std::string get_hash(const std::string& password) {
  CryptoPP::SHA512 sha;
  CryptoPP::byte digest[CryptoPP::SHA512::DIGESTSIZE];

  sha.CalculateDigest(digest, reinterpret_cast<CryptoPP::byte const*>(password.c_str()),
                      password.length());

  CryptoPP::HexEncoder encoder;
  std::string result;
  encoder.Attach(new CryptoPP::StringSink(result));
  encoder.Put(digest, sizeof(digest));
  encoder.MessageEnd();

  return result;
}

void test_login() {
  std::vector<User> users{{101, "scarface",
                           "07A8D53ADAB635ADDF39BAEACFB799FD7C5BFDEE365F3AA721B7E25B54A4E87D419ADDE"
                           "A34BC3073BAC472DCF4657E50C0F6781DDD8FE883653D10F7930E78FF",
                           "Tony", "Montana"},
                          {202, "neo",
                           "C2CC277BCC10888ECEE90F0F09EE9666199C2699922EFB41EA7E88067B2C075F3DD3FBF"
                           "3CFE9D0EC6173668DD83C111342F91E941A2CADC46A3A814848AA9B05",
                           "Thomas", "Anderson"},
                          {303, "godfather",
                           "0EA7A0306FE00CD22DF1B835796EC32ACC702208E0B052B15F9393BCCF5EE9ECD8BAAF2"
                           "7840D4D3E6BCC3BB3B009259F6F73CC77480C065DDE67CD9BEA14AA4D",
                           "Vito", "Corleone"}};

  std::string username, password;
  std::cout << "Username: ";
  std::cin >> username;
  std::cout << "Password: ";
  std::cin >> password;

  auto hash = get_hash(password);

  auto pos = std::find_if(users.cbegin(), users.cend(), [username, hash](const User& u) {
    return u.username == username && u.password == hash;
  });
  if (pos != users.cend()) {
    std::cout << "Login successful!" << std::endl;
  } else {
    std::cerr << "Invalid username or password" << std::endl;
  }
}

template <class Hash>
std::string calc_hash(const std::filesystem::path& filepath) {
  std::string digest;
  Hash hash;

  CryptoPP::FileSource source(
      filepath.c_str(), true,
      new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));

  return digest;
}

void test_calc_hash() {
  std::string path{"../crypto.cc"};
  try {
    std::cout << "SHA1: " << calc_hash<CryptoPP::SHA1>(path) << std::endl;
    std::cout << "SHA256: " << calc_hash<CryptoPP::SHA256>(path) << std::endl;
    std::cout << "MD5: " << calc_hash<CryptoPP::Weak::MD5>(path) << std::endl;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}

void encrypt_file(const std::filesystem::path& source_file, const std::filesystem::path& dest_file,
                  const std::string& password) {
  CryptoPP::FileSource source(
      source_file.c_str(), true,
      new CryptoPP::DefaultEncryptorWithMAC((CryptoPP::byte*)password.c_str(), password.size(),
                                            new CryptoPP::FileSink(dest_file.c_str())));
}

void decrypt_file(const std::filesystem::path& source_file, const std::filesystem::path& dest_file,
                  const std::string& password) {
  CryptoPP::FileSource source(
      source_file.c_str(), true,
      new CryptoPP::DefaultDecryptorWithMAC((CryptoPP::byte*)password.c_str(), password.size(),
                                            new CryptoPP::FileSink(dest_file.c_str())));
}

void test_crypt_file() {
  encrypt_file("../crypto.cc", "crypto.cc.enc", "passwd");
  decrypt_file("crypto.cc.enc", "crypto.cc.dec", "passwd");
}

void encode(const std::filesystem::path& filepath, const CryptoPP::BufferedTransformation& bt) {
  CryptoPP::FileSink file(filepath.c_str());
  bt.CopyTo(file);
  file.MessageEnd();
}

void encode_private_key(const std::filesystem::path& filepath,
                        const CryptoPP::RSA::PrivateKey& key) {
  CryptoPP::ByteQueue queue;
  key.DEREncodePrivateKey(queue);
  encode(filepath, queue);
}

void encode_public_key(const std::filesystem::path& filepath, const CryptoPP::RSA::PublicKey& key) {
  CryptoPP::ByteQueue queue;
  key.DEREncodePublicKey(queue);
  encode(filepath, queue);
}

void decode(const std::filesystem::path& filepath, CryptoPP::BufferedTransformation& bt) {
  CryptoPP::FileSource file(filepath.c_str(), true);
  file.TransferTo(bt);
  bt.MessageEnd();
}

void decode_private_key(const std::filesystem::path& filepath, CryptoPP::RSA::PrivateKey& key) {
  CryptoPP::ByteQueue queue;
  decode(filepath, queue);
  key.BERDecodePrivateKey(queue, false, queue.MaxRetrievable());
}

void decode_public_key(const std::filesystem::path& filepath, CryptoPP::RSA::PublicKey& key) {
  CryptoPP::ByteQueue queue;
  decode(filepath, queue);
  key.BERDecodePublicKey(queue, false, queue.MaxRetrievable());
}

void generate_keys(const std::filesystem::path& private_key_path,
                   const std::filesystem::path& public_key_path,
                   CryptoPP::RandomNumberGenerator& rng) {
  try {
    CryptoPP::RSA::PrivateKey rsa_private;
    rsa_private.GenerateRandomWithKeySize(rng, 3072);

    CryptoPP::RSA::PublicKey rsa_public(rsa_private);

    encode_private_key(private_key_path, rsa_private);
    encode_public_key(public_key_path, rsa_public);
  } catch (const CryptoPP::Exception& e) {
    std::cerr << e.what() << std::endl;
  }
}

void rsa_sign_fiile(const std::filesystem::path& filepath,
                    const std::filesystem::path& private_key_path,
                    const std::filesystem::path& signature_path,
                    CryptoPP::RandomNumberGenerator& rng) {
  CryptoPP::RSA::PrivateKey private_key;
  decode_private_key(private_key_path, private_key);
  CryptoPP::RSASSA_PKCS1v15_SHA_Signer signer(private_key);

  CryptoPP::FileSource file_source(
      filepath.c_str(), true,
      new CryptoPP::SignerFilter(rng, signer, new CryptoPP::FileSink(signature_path.c_str())));
}

bool rsa_verify_file(const std::filesystem::path& filepath,
                    const std::filesystem::path& public_key_path,
                     const std::filesystem::path& signature_path) {
  CryptoPP::RSA::PublicKey public_key;
  decode_public_key(public_key_path.c_str(), public_key);

  CryptoPP::RSASSA_PKCS1v15_SHA_Verifier verifier(public_key);

  CryptoPP::FileSource signature_file(signature_path.c_str(), true);

  if (signature_file.MaxRetrievable() != verifier.SignatureLength())
    return false;

  CryptoPP::SecByteBlock signature(verifier.SignatureLength());
  signature_file.Get(signature,signature.size());

  auto* verifier_filter = new CryptoPP::SignatureVerificationFilter(verifier);
  verifier_filter->Put(signature, verifier.SignatureLength());

  CryptoPP::FileSource file_source(filepath.c_str(), true, verifier_filter);

  return verifier_filter->GetLastResult();
}

void test_sign_file() {
  CryptoPP::AutoSeededRandomPool rng;

  generate_keys("rsa-private.key", "rsa-public.key", rng);

  rsa_sign_fiile("sample.txt", "rsa-private.key", "sample.sign", rng);

  auto success = rsa_verify_file("sample.txt", "rsa-public.key", "sample.sign");
  assert(success);
}

int main() {
  test_caesar();
  test_vigenere();
  test_base64();

  // test_login();
  // test_calc_hash();
  // test_crypt_file();
  test_sign_file();
}
