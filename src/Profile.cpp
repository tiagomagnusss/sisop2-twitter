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
        std::cout << "Saving " << profile.getUsername() << " with " << profile.followers.size() << " followers and " << profile.following.size() << " following" << std::endl;
        database.push_back({{"username", username}, {"followers", profile.followers}, {"following", profile.following}});
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
        pf.setFollowers( item.value()["followers"].get<std::list<std::string>>() );
        pf.setFollowing( item.value()["following"].get<std::list<std::string>>() );


        profiles.insert(std::pair<std::string, Profile>(pf.getUsername(), pf));
    }

    printf("%i profiles loaded successfully\n", (int) profiles.size());
}

Profile Profile::get_user(std::string username)
{
    if ( user_exists(username) )
    {
        std::cout << "Getting user " << username << std::endl;
        return profiles.at(username);
    }

    create_user(username);
    return profiles.at(username);
}

std::string Profile::getUsername()
{
    return _username;
}

void Profile::create_user(std::string username)
{
    std::cout << "Creating user " << username << std::endl;

    _username = username;
    followers = std::list<std::string>();
    following = std::list<std::string>();

    profiles.insert(std::pair<std::string, Profile>(username, *this));
    saveProfiles();
}

bool Profile::user_exists(std::string username)
{
    return profiles.find(username) != profiles.end();
}

void Profile::follow_user(std::string username, std::string follow)
{
    profiles.at(username).following.push_back(follow);
    profiles.at(follow).followers.push_back(username);

    // remove duplicatas
    profiles.at(username).following.unique();
    profiles.at(follow).followers.unique();
}

void Profile::setUsername( std::string username )
{
    _username = username;
}

void Profile::setFollowers( std::list<std::string> followers )
{
    followers = followers;
}

void Profile::setFollowing( std::list<std::string> following )
{
    following = following;
}
