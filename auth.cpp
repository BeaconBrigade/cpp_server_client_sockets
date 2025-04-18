#include <cstdio>
#include <cstring>
#include <iostream>
#include <iterator>
#include <openssl/evp.h>
#include <ostream>
#include <sodium.h>
#include <sodium/crypto_pwhash.h>
#include <sqlite3.h>
#include <string>

int create_user(sqlite3 *DB, std::string *username, std::string *password) {

  sqlite3_stmt *stmt;

  const char *check_exists_query =
      "SELECT COUNT(*) FROM users WHERE username = ?;";

  if (sqlite3_prepare_v2(DB, check_exists_query, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    std::cerr << "unable to prepare check_exists_query" << sqlite3_errmsg(DB)
              << std::endl;
  }

  sqlite3_bind_text(stmt, 1, username->c_str(), -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    int count = sqlite3_column_int(stmt, 0);
    if (count > 0) {
      std::cerr << "USER EXISTS" << std::endl;
      return 1;
    }
  }

  sqlite3_finalize(stmt);

  char hashed_password[crypto_pwhash_STRBYTES];

  const unsigned char *key =
      reinterpret_cast<const unsigned char *>(username->c_str());

  if (crypto_pwhash_str(hashed_password, password->c_str(), password->length(),
                        crypto_pwhash_OPSLIMIT_SENSITIVE,
                        crypto_pwhash_MEMLIMIT_SENSITIVE) != 0) {

    std::cout << "out of memory from crypto_pwhash_str inside create_user()"
              << std::endl;
    /* out of memory */
  }

  if (crypto_pwhash_str_verify(hashed_password, password->c_str(),
                               password->length()) != 0) {
    std::cerr << "wrong password bitch" << std::endl;
    return 3;
    /* wrong password */
  }

  std::cout << "successfully verified password" << std::endl;

  const char *create_user_query =
      "INSERT INTO users( username, hashed_pswd ) values( ?, ? );";

  if (sqlite3_prepare_v2(DB, create_user_query, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    std::cerr << "unable to prepare create_user_query" << sqlite3_errmsg(DB)
              << std::endl;
  }

  std::cerr << "right before binds" << std::endl;

  std::cout << "last char of hashed_password: " << hashed_password[127]
            << std::endl;

  sqlite3_bind_text(stmt, 1, username->c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, hashed_password, -1, SQLITE_STATIC);

  int res = sqlite3_step(stmt);

  std::cout << "thjios was res: " << res << std::endl;
  std::cout << sqlite3_errmsg(DB) << std::endl;

  sqlite3_finalize(stmt);

  std::fill(std::begin(hashed_password), std::end(hashed_password), '\0');
  std::fill(username->begin(), username->end(), '\0');
  std::fill(password->begin(), password->end(), '\0');

  return 0;
}

int login(sqlite3 *DB, std::string *username, std::string *password) {

  std::cout << "inside login" << std::endl;

  sqlite3_stmt *stmt;

  const char *retrieve_hashed_pswd =
      "SELECT hashed_pswd FROM users WHERE username = ?";

  if (sqlite3_prepare_v2(DB, retrieve_hashed_pswd, -1, &stmt, nullptr) !=
      SQLITE_OK) {
    std::cerr << "unable to prepare retrieve_hashed_pswd" << sqlite3_errmsg(DB)
              << std::endl;
  }

  sqlite3_bind_text(stmt, 1, username->c_str(), -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) != SQLITE_ROW) {
    std::cerr << "no match" << std::endl;
    return 1;
  }

  std::cout << sqlite3_errmsg(DB) << std::endl;
  const unsigned char *password_hash = sqlite3_column_text(stmt, 0);
  std::cerr << password_hash << std::endl;

  if (crypto_pwhash_str_verify((const char*)password_hash, password->c_str(),
                               password->length()) != 0) {
    /* wrong password */
    std::cerr << "YOUUU SHALL NOTTTT PASSSSSSSSSSS" << std::endl;
  }
  
  std::cout << "SUCCESSFUL LOGIN YIPPIEEE" << std::endl;
  
  sqlite3_finalize(stmt);
  std::fill(username->begin(), username->end(), '\0');
  std::fill(password->begin(), password->end(), '\0');

  return 0;
}

int confirm_credentials() {

  sqlite3 *DB;

  int exit = 0;

  exit = sqlite3_open("term_chat_users.db", &DB);

  if (exit != 0) {
    std::cerr << "couldn't open database" << std::endl;
    return 1;
  }

  std::string username;
  std::string password;

  std::cout << "enter username:" << std::endl;
  std::cin >> username;
  std::cout << "enter password:" << std::endl;
  std::cin >> password;

  if (login(DB, &username, &password)) {
		std::cerr << "invalid creds" << std::endl;
	}

	return 0;
}
