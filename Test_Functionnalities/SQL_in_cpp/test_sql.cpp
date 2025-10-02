#include <iostream>
#include <sqlite3.h>


// function to execute an SQL query
void execute_SQL(sqlite3* db, const std::string& sql)
{
    char* error_message;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_message) != SQLITE_OK){
        std::cerr << "Error executing SQL: " << error_message << std::endl;
        sqlite3_free(error_message);
    }
}

// function to create the table
void create_table(sqlite3* db)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS users ("
                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                 "name TEXT NOT NULL);";
    execute_SQL(db, sql);
}

// function to add a user
void add_user(sqlite3* db, const std::string& name)
{
    std::string sql = "INSERT INTO users (name) VALUES ('" + name + "');";
    execute_SQL(db, sql);
    std::cout << "User '" << name << "' added successfully!\n";
}

// callback function for retrieving users
static int callback(void* not_used, int argc, char** argv, char** col_names)
{
    for (int i = 0; i < argc; i++){
        std::cout << col_names[i] << ": " << (argv[i] ? argv[i] : "NULL") << " | ";
    }
    std::cout << std::endl;
    return 0;
}

// function to retrieve all users
void get_users(sqlite3* db)
{
    std::string sql = "SELECT * FROM users;";
    std::cout << "\nUsers in Database:\n";
    execute_SQL(db, sql);
    sqlite3_exec(db, sql.c_str(), callback, nullptr, nullptr);
}

// function to update a user's name
void update_user(sqlite3* db, int user_id, const std::string& new_name)
{
    std::string sql = "UPDATE users SET name = '" + new_name + "' WHERE id = " + std::to_string(user_id) + ";";
    execute_SQL(db, sql);
    std::cout << "User ID " << user_id << " updated to '" << new_name << "'!\n";
}

// function to delete a user
void delete_user(sqlite3* db, int user_id)
{
    std::string sql = "DELETE FROM users WHERE id = " + std::to_string(user_id) + ";";
    execute_SQL(db, sql);
    std::cout << "User ID " << user_id << " deleted!\n";
}

// function to get a valid integer input
int get_valid_int(const std::string& prompt)
{
    int value;
    while (true){
        std::cout << prompt;
        std::cin >> value;

        if (std::cin.fail()){
            std::cin.clear();  // clear error flag
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // discard invalid input
            std::cout << "Invalid input. Please enter a valid number.\n";
        }
        else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // remove extra characters
            return value;
        }
    }
}

int main()
{
    // open SQLite database
    sqlite3* db;
    if (sqlite3_open("my_database.db", &db) != SQLITE_OK){
        std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    // create table
    create_table(db);

    while (true){
        std::cout << "\nSQLite User Management:\n";
        std::cout << "1. Add User\n";
        std::cout << "2. View Users\n";
        std::cout << "3. Update User\n";
        std::cout << "4. Delete User\n";
        std::cout << "5. Exit\n";
        
        // verify input is an integer between 1 and 5
        int choice = get_valid_int("Choose an option: ");

        if (choice == 1){
            std::string name;
            std::cout << "Enter the user's name: ";
            getline(std::cin, name);
            add_user(db, name);
        }
        else if (choice == 2){
            get_users(db);
        }
        else if (choice == 3){
            int user_id = get_valid_int("Enter user ID to update: ");
            std::string new_name;
            std::cout << "Enter new name: ";
            getline(std::cin, new_name);
            update_user(db, user_id, new_name);
        } 
        else if (choice == 4){
            int user_id = get_valid_int("Enter user ID to delete: ");
            delete_user(db, user_id);
        } 
        else if (choice == 5){
            std::cout << "Exiting...\n";
            break;
        }
    }

    // close database
    sqlite3_close(db);
    return 0;
}