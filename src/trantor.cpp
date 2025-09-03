// example - wiring the ORM with SQLite adapter
#include "driver/sqlite_connection.hpp"
#include "dialect/sqlite_dialect.hpp"
#include "connection.hpp"
#include "table.hpp"
#include "column.hpp"
#include <iostream>
#include <variant>

// define a simple row class and table (example)
struct User {
    int id{};
    std::string name;
    int age;
    int getId() const { return id; }
    void setId(int v) { id = v; }
};

using namespace trantor;

// define columns (member pointer version)
static constexpr FixedLengthString userTableName("users");

using IdColumn = Column<FixedLengthString("id"), &User::id, column_constraint::NotNull<>>;
using NameColumn = Column<FixedLengthString("name"), &User::name, column_constraint::NotNull<>>;
using AgeColumn = Column<FixedLengthString("age"), &User::age>;

using UserTable = Table<userTableName, User, IdColumn, NameColumn, AgeColumn>;

int main() {
    // create connection
    auto maybeConn = SqliteConnection::create("example.db");
    // prefer get_if to avoid template overload ambiguity
    if (auto errPtr = std::get_if<Error>(&maybeConn)) {
        std::cerr << "Unable to open DB: " << *errPtr << std::endl;
        return 1;
    }

    // now safely extract the connection (move)
    auto connPtr = std::move(std::get<std::unique_ptr<SqliteConnection>>(maybeConn));
    if (!connPtr) {
        std::cerr << "unexpected null connection" << std::endl;
        return 1;
    }

    auto dialect = std::make_unique<SqliteDialect>();
    OrmConnection<UserTable> orm(std::move(connPtr), std::move(dialect));

    if (auto err = orm.createTables(true)) {
        std::cerr << "createTables error: " << *err << std::endl;
        return 1;
    }

    User u;
    u.name = "Alyx";
    u.age = 28;

    if (auto err = orm.insert(u)) {
        std::cerr << "insert error: " << *err << std::endl;
        return 1;
    }

    std::cout << "Inserted" << std::endl;
    return 0;
}