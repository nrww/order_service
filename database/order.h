#ifndef AUTHOR_H
#define AUTHOR_H

#include <string>
#include <vector>
#include "Poco/JSON/Object.h"
#include <optional>

namespace database
{
    class Order{
        private:
            long _id;
            long _client_id;
            long _service_id;
            std::string _status;
            std::string _content;
            std::string _date;


        public:

            static Order fromJSON(const std::string & str);

            const long        &get_id() const;
            const std::string &get_status() const;
            const std::string &get_content() const;
            const std::string &get_date() const;
            const long        &get_service_id() const;
            const long        &get_client_id() const;


            long        &id();
            std::string &status();
            std::string &content();
            std::string &date();
            long        &service_id();
            long        &client_id();

            static void init();
            void update_in_mysql();
            bool save_to_mysql();
            static bool delete_in_mysql(long id);

            static std::optional<Order> read_by_id(long id);
            static std::vector<Order> read_by_client_id(long client_id);

            Poco::JSON::Object::Ptr toJSON() const;

    };
}

#endif