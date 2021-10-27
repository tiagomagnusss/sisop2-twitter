#include "../include/Profile.h"

std::mutex mutexFile, mutexMemory;

static std::string DB_PATH = "database/profiles.json";
std::map<std::string, Profile> profiles;

Profile::Profile()
{}

Profile::Profile(std::string username)
{
    if (!user_exists(username))
    {
        create_user(username);
    }
}

Profile::~Profile()
{
    // salva pra evitar perda de dados
}

void Profile::saveProfiles()
{
    nlohmann::json database;
    //printf("Saving %i profiles...\n", (int) profiles.size());
    mutexFile.lock();
    // itera sobre os perfis já salvos pelo server
    for (auto pair : profiles)
    {
        std::string username = pair.first;
        Profile profile = pair.second;
        database.push_back({{"username", username}, {"followers", profile.followers}, {"following", profile.following}});
    }

    // salva em disco
    // precisa usar ofstream segundo o autor
    std::ofstream stream(DB_PATH);
    // TODO: ver qual identação fica melhor
    stream << database.dump(2);
    stream.close();

    mutexFile.unlock();
    profiles.clear();

    //printf("Profiles saved successfully\n");
}

void Profile::loadProfiles()
{
    nlohmann::json database;
    //printf("Loading profiles...\n");
    mutexFile.lock();
    std::ifstream stream(DB_PATH);

    stream >> database;
    stream.close();
    mutexFile.unlock();
    // preenche a lista
    for (const auto item : database.items())
    {
        Profile pf = Profile();
        pf.setUsername(item.value()["username"].get<std::string>());
        pf.setFollowers(item.value()["followers"].get<std::list<std::string>>());
        pf.setFollowing(item.value()["following"].get<std::list<std::string>>());

        profiles.insert(std::pair<std::string, Profile>(pf.getUsername(), pf));
        //profiles.emplace(pf);
    }

    //printf("%i profiles loaded successfully\n", (int) profiles.size());
}

Profile *Profile::get_user(std::string username)
{
    if (user_exists(username))
    {
        std::cout << "Getting user " << username << std::endl;
        return &profiles.at(username);
    }

    create_user(username);
    return &profiles.at(username);
}

std::string Profile::getUsername()
{
    return _username;
}

void Profile::create_user(std::string username)
{
    mutexMemory.lock();
    std::cout << "Creating user " << username << std::endl;

    _username = username;
    followers = std::list<std::string>();
    following = std::list<std::string>();

    profiles.insert(std::pair<std::string, Profile>(username, *this));
    mutexMemory.unlock();
}

void Profile::follow_user(std::string username, std::string follow)
{
    mutexMemory.lock();
    bool repeated = false;
    for ( std::string userFollowing : profiles.at(username).following )
    {
        if ( follow == userFollowing )
        {
            repeated = true;
            break;
        }
    }

    if ( !repeated )
    {
        profiles.at(username).following.push_back(follow);
        profiles.at(follow).followers.push_back(username);
    }
    mutexMemory.unlock();
}

bool Profile::user_exists(std::string username)
{
    return profiles.find(username) != profiles.end();
}

void Profile::setUsername(std::string username)
{
    _username = username;
}

void Profile::setFollowers(std::list<std::string> newFollowers)
{
    mutexMemory.lock();
    followers = newFollowers;
    mutexMemory.unlock();
}

void Profile::setFollowing(std::list<std::string> newFollowing)
{
    mutexMemory.lock();
    following = newFollowing;
    mutexMemory.unlock();
}
