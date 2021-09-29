#ifndef PROFILE_H
#define PROFILE_H

#include <iostream>
#include <fstream>
#include <list>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <regex>
#include <nlohmann/json.hpp>

class Profile
{
    public:
        Profile();
        Profile(std::string username);
        ~Profile();

        void create_user(std::string username);
        bool user_exists(std::string username);
        Profile get_user(std::string username);
        std::string getUsername();

        void follow_user(std::string username, std::string follow);

        void setUsername(std::string username);
        void setFollowers(std::list<std::string> followers);
        void setFollowing(std::list<std::string> followers);

        void saveProfiles();
        void loadProfiles();

        std::list<std::string> followers;
        std::list<std::string> following;
    private:
        std::string _username;

};

#endif