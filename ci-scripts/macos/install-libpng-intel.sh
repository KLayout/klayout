
# Install homebrew in /usr/local
arch -x86_64 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Switch to /usr/local version
eval "$(/usr/local/homebrew/bin/brew shellenv)"

# install libpng for intel on /usr/local
which brew
arch -x86_64 brew install -v libpng

