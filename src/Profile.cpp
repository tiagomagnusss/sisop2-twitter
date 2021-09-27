#include "../include/Profile.h"

static std::string DB_PATH = "database/profiles.json";
std::map<std::string, Profile> profiles;

Profile::Profile()
{}

Profile::Profile(std::string username)
{
    if ( !user_exists(username) )
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
    printf("Saving %i profiles...\n", (int) profiles.size());

    // itera sobre os perfis já salvos pelo server
    for ( auto pair : profiles )
    {
        std::string username = pair.first;
        Profile profile = pair.second;
        database.push_back({{"username", username}, {"followers", profile.getFollowers()}, {"following", profile.getFollowing()}});
    }

    // salva em disco
    // precisa usar ofstream segundo o autor
    std::ofstream stream(DB_PATH);
    // TODO: ver qual identação fica melhor
    stream << database.dump(2);
    stream.close();

    printf("Profiles saved successfully\n");
}

void Profile::loadProfiles()
{
    nlohmann::json database;
    printf("Loading profiles...\n");

    std::ifstream stream(DB_PATH);

    stream >> database;
    stream.close();

    // preenche a lista
    for ( const auto item : database.items() )
    {
        Profile pf = Profile();
        pf.setUsername(item.value()["username"].get<std::string>());
        pf.setFollowers(item.value()["followers"].get<std::list<std::string>>());
        pf.setFollowing(item.value()["followers"].get<std::list<std::string>>());

        profiles.insert(std::pair<std::string, Profile>(pf.getUsername(), pf));
        //profiles.emplace(pf);
    }

    printf("%i profiles loaded successfully\n", (int) profiles.size());
}

std::string Profile::getUsername()
{
    return _username;
}

void Profile::create_user(std::string username)
{
    std::cout << "Creating user " << username << std::endl;

    _username = username;
    setFollowers(std::list<std::string>());
    setFollowing(std::list<std::string>());

    profiles.insert(std::pair<std::string, Profile>(username, *this));
    saveProfiles();
}

bool Profile::user_exists(std::string username)
{
    return profiles.find(username) != profiles.end();
}

std::list<std::string> Profile::getFollowers()
{
    return _followers;
}

std::list<std::string> Profile::getFollowing()
{
    return _following;
}

void Profile::setUsername( std::string username )
{
    _username = username;
}

void Profile::setFollowers( std::list<std::string> followers )
{
    _followers = followers;
}

void Profile::setFollowing( std::list<std::string> following )
{
    _following = following;
}
