# 🗄️ SQLite User Management in C++

This program demonstrates how to create and manage a simple **SQLite database** in C++ using the **SQLite3 C API**.  
It provides a command-line interface to **add**, **view**, **update**, and **delete** users from a local database file (`my_database.db`).

---

## ⚙️ Features

- Automatically creates a `users` table if it doesn’t exist  
- Supports:
  - Adding users  
  - Viewing all users  
  - Updating user names  
  - Deleting users  
- Simple text-based menu interface  
- Input validation for integers  
- Error handling for SQL execution  

---

## 🧩 Code Overview

| Function | Description |
|-----------|--------------|
| `execute_SQL()` | Executes any given SQL command and handles errors |
| `create_table()` | Creates the `users` table if not already present |
| `add_user()` | Inserts a new user into the database |
| `get_users()` | Retrieves and prints all users in the table |
| `update_user()` | Updates an existing user’s name |
| `delete_user()` | Removes a user from the database by ID |
| `get_valid_int()` | Ensures valid numeric input from the user |
| `main()` | Initializes the database, runs menu loop, and handles user commands |

---

## 🧠 Database Schema

| Field | Type | Description |
|--------|------|-------------|
| `id` | INTEGER PRIMARY KEY AUTOINCREMENT | Unique user ID |
| `name` | TEXT NOT NULL | User name |

---

## 🛠️ Compilation

Make sure you have SQLite3 development libraries installed.  
Then compile with:

```bash
g++ -std=c++17 -Wall -Wfatal-errors -o test_sql.o -c test_sql.cpp -L/opt/homebrew/lib -lsqlite3
```

---

## ▶️ Usage

```bash
./test_sql.x
```

You’ll see a simple interactive menu:
```yaml
SQLite User Management:
1. Add User
2. View Users
3. Update User
4. Delete User
5. Exit
Choose an option:
```

```yaml
Choose an option: 1
Enter the user's name: Alice
User 'Alice' added successfully!

Choose an option: 2
Users in Database:
id: 1 | name: Alice |

Choose an option: 5 
Exiting...
```
