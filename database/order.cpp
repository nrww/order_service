#include "order.h"
#include "database.h"
#include "../config/config.h"

#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Data/SessionFactory.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Dynamic/Var.h>

#include <sstream>
#include <exception>

using namespace Poco::Data::Keywords;
using Poco::Data::Session;
using Poco::Data::Statement;
#define DATE_FORMAT "\'DD.MM.YYYY HH24:MI:SS\'"

namespace database
{

    void Order::init()
    {
        try
        {

            Poco::Data::Session session = database::Database::get().create_session();
            Statement create_stmt(session);
            create_stmt << "CREATE TABLE IF NOT EXISTS `order` ("
                        << "`order_id` INT NOT NULL AUTO_INCREMENT,"
                        << "`status` VARCHAR(256) NOT NULL,"
                        << "`content` VARCHAR(4000),"
                        << "`client_id` INT NOT NULL,"
                        << "`date` DATETIME NOT NULL,"
                        << "`service_id` INT NOT NULL,"
                        << "PRIMARY KEY (`order_id`),"
                        << "FOREIGN KEY (`service_id`) REFERENCES `service` (`service_id`),"
                        << "FOREIGN KEY (`client_id`) REFERENCES `user` (`user_id`)"
                        << ");",
                now;
            
        }

        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << "connection:" << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {

            std::cout << "statement:" << e.what() << std::endl;
            throw;
        }
    }

    void Order::update_in_mysql()
    {
        try
        {
            Poco::Data::Session session = database::Database::get().create_session();
            Poco::Data::Statement update(session);

            update  << "UPDATE `order` "
                    << "SET `status` = ?, `content` = ?"
                    << "WHERE `order_id` = ? ;",
                use(_status),
                use(_content),
                use(_id),
                now;

            update.execute();

            std::cout << "updated: " << _id << std::endl;
        }
        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << "connection:" << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {

            std::cout << "statement:" << e.what() << std::endl;
            throw;
        }
    }

    bool Order::delete_in_mysql(long id)
    {
        try
        {
            Poco::Data::Session session = database::Database::get().create_session();
            Statement del(session);
            
            Order a;
            del << "DELETE FROM `order` WHERE `order_id` = ?;",
                use(id),
                range(0, 1); 
            del.execute();
            std::cout << "deleted: " << id << std::endl;
            return true;
        }

        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << "connection:" << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {

            std::cout << "statement:" << e.what() << std::endl;
            throw;
        }
        return false;
    }

    Poco::JSON::Object::Ptr Order::toJSON() const
    {
        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();

        root->set("id", _id);
        root->set("status", _status);
        root->set("content", _content);
        root->set("date", _date);
        root->set("service_id", _service_id);
        root->set("client_id", _client_id);

        return root;
    }

    Order Order::fromJSON(const std::string &str)
    {
        Order user;
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var result = parser.parse(str);
        Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();

        user.id() = object->getValue<long>("id");
        user.status() = object->getValue<std::string>("status");
        user.content() = object->getValue<std::string>("content");
        user.date() = object->getValue<std::string>("date");
        user.service_id() = object->getValue<long>("service_id");
        user.client_id() = object->getValue<long>("client_id");


        return user;
    }

    std::optional<Order> Order::read_by_id(long id)
    {
        try
        {
            Poco::Data::Session session = database::Database::get().create_session();
            Statement select(session);
            Order a;
            select  << "SELECT `order_id`, `status`, `content`, TO_CHAR(`date`, " << DATE_FORMAT << "), `service_id`, `client_id`"
                    << "FROM `order`" 
                    << "WHERE `order_id` = ? ;",
                into(a._id),
                into(a._status),
                into(a._content),
                into(a._date),
                into(a._service_id),
                into(a._client_id),
                use(id),
                range(0, 1); //  iterate over result set one row at a time

            select.execute();
            Poco::Data::RecordSet rs(select);
            if (rs.moveFirst()) return a;
        }

        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << "connection:" << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {

            std::cout << "statement:" << e.what() << std::endl;
            throw;
        }
        return {};
    }

    std::vector<Order> Order::read_by_client_id(long client_id)
    {
        std::vector<Order> result;

        try
        {
            Poco::Data::Session session = database::Database::get().create_session();
            Statement select(session);
            Order a;
            select  << "SELECT `order_id`, `status`, `content`, TO_CHAR(`date`, " << DATE_FORMAT << "), `service_id`, `client_id` " 
                    << "FROM `order` " 
                    << "WHERE `client_id` = ? ;",
                into(a._id),
                into(a._status),
                into(a._content),
                into(a._date),
                into(a._service_id),
                into(a._client_id),
                use(client_id),
                range(0, 1); //  iterate over result set one row at a time

            while (!select.done())
            {
                if (select.execute())
                    result.push_back(a);
            }
        }

        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << "connection:" << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {

            std::cout << "statement:" << e.displayText() << std::endl;
            throw;
        }
        return result;
    }

    bool Order::save_to_mysql()
    {

        try
        {
            Poco::Data::Session session = database::Database::get().create_session();
            Poco::Data::Statement insert(session);

            insert  << "INSERT INTO `order` (`status`, `content`, `date`, `service_id`, `client_id`)"
                    << "VALUES(?, ?, NOW(), ?, ?)",
                use(_status),
                use(_content),
                use(_service_id),
                use(_client_id);

            insert.execute();

            Poco::Data::Statement select(session);
            select << "SELECT LAST_INSERT_ID()",
                into(_id),
                range(0, 1); //  iterate over result set one row at a time

            if (!select.done())
            {
                select.execute();
            }
            std::cout << "inserted: " << _id << std::endl;
            return true;
        }
        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << "connection: " << e.what() << std::endl;
            throw;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {
            std::cout << "statement: " << e.displayText() << std::endl;
            throw;
        }
        return false;
    }

    const long &Order::get_id() const
    {
        return _id;
    }

    const std::string &Order::get_status() const
    {
        return _status;
    }

    const std::string &Order::get_content() const
    {
        return _content;
    }

    const std::string &Order::get_date() const
    {
        return _date;
    }

    const long &Order::get_service_id() const
    {
        return _service_id;
    }

    const long &Order::get_client_id() const
    {
        return _client_id;
    }


    long &Order::id()
    {
        return _id;
    }

    std::string &Order::status()
    {
        return _status;
    }

    std::string &Order::content()
    {
        return _content;
    }

    std::string &Order::date()
    {
        return _date;
    }

    long &Order::service_id()
    {
        return _service_id;
    }

    long &Order::client_id()
    {
        return _service_id;
    }
}