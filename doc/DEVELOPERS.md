# Developer guide

For Fedora 40

## Install PostgreSql

``` sh
dnf install postgresql-server
```

, this will install PostgreSQL 15.

## Install pgmoneta

### Pre-install

#### Basic dependencies

``` sh
dnf install gcc cmake make libev libev-devel
```

### Build

``` sh
cd /usr/local
git clone https://github.com/jesperpedersen/pgprtdbg.git
cd pgprtdbg
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
make install
```

This will install [**pgprtdbg**](https://github.com/jesperpedersen/pgprtdbg) in the `/usr/local` hierarchy with the debug profile.

### Check version

You can navigate to `build/src` and execute `./pgprtdbg -?` to make the call. Alternatively, you can install it into `/usr/local/` and call it directly using:

``` sh
pgprtdbg -?
```

If you see an error saying `error while loading shared libraries: libpgprtdbg.so.0: cannot open shared object` running the above command. you may need to locate where your `libpgprtdbg.so.0` is. It could be in `/usr/local/lib` or `/usr/local/lib64` depending on your environment. Add the corresponding directory into `/etc/ld.so.conf`.

To enable these directories, you would typically add the following lines in your `/etc/ld.so.conf` file:

``` sh
/usr/local/lib
/usr/local/lib64
```

Remember to run `ldconfig` to make the change effective.

## Setup pgprtdbg

Let's give it a try. The basic idea here is that we will use two users: one is `postgres`, which will run PostgreSQL, and one is [**pgprtdbg**](https://github.com/jesperpedersen/pgprtdbg), which will run [**pgprtdbg**](https://github.com/jesperpedersen/pgprtdbg) to debug the PostgreSQL protocol.

In many installations, there is already an operating system user named `postgres` that is used to run the PostgreSQL server. You can use the command

``` sh
getent passwd | grep postgres
```

to check if your OS has a user named postgres. If not use

``` sh
useradd -ms /bin/bash postgres
passwd postgres
```

If the postgres user already exists, don't forget to set its password for convenience.

### 1. postgres

Open a new window, switch to the `postgres` user. This section will always operate within this user space.

``` sh
sudo su -
su - postgres
```

#### Initialize cluster

If you use dnf to install your postgresql, chances are the binary file is in `/usr/bin/`

``` sh
export PATH=/usr/bin:$PATH
initdb /tmp/pgsql
```

#### Remove default acess

Remove last lines from `/tmp/pgsql/pg_hba.conf`

``` ini
host    all             all             127.0.0.1/32            trust
host    all             all             ::1/128                 trust
host    replication     all             127.0.0.1/32            trust
host    replication     all             ::1/128                 trust
```

#### Add access for users and a database

Add new lines to `/tmp/pgsql/pg_hba.conf`

``` ini
host    mydb             myuser          127.0.0.1/32            scram-sha-256
host    mydb             myuser          ::1/128                 scram-sha-256
```

#### Set password_encryption

Set `password_encryption` value in `/tmp/pgsql/postgresql.conf` to be `scram-sha-256`

``` sh
password_encryption = scram-sha-256
```

For version up till 13, the default is `md5`, while for version 14 and above, it is `scram-sha-256`. Therefore, you should ensure that the value in `/tmp/pgsql/postgresql.conf` matches the value in `/tmp/pgsql/pg_hba.conf`.

#### Start PostgreSQL

``` sh
pg_ctl  -D /tmp/pgsql/ start
```

Here, you may encounter issues such as the port being occupied or permission being denied. If you experience a failure, you can go to `/tmp/pgsql/log` to check the reason.

You can use

``` sh
pg_isready
```

to test

#### Add users and a database

``` sh
createuser -P myuser
createdb -E UTF8 -O myuser mydb
```

#### Verify access

For the user `myuser` (standard) use `mypass`

``` sh
psql -h localhost -p 5432 -U myuser mydb
\q
```

#### Add pgprtdbg user

``` sh
sudo su -
useradd -ms /bin/bash pgprtdbg
passwd pgprtdbg
exit
```

### 2. pgprtdbg

Open a new window, switch to the `pgprtdbg` user. This section will always operate within this user space.

``` sh
sudo su -
su - pgprtdbg
```

#### Create pgprtdbg configuration

Create the `pgprtdbg.conf` configuration file to use when running [**pgprtdbg**](https://github.com/pgprtdbg/pgprtdbg).

``` ini
cat > pgprtdbg.conf
[pgprtdbg]
host = *
port = 2346

output = pgprtdbg.out

log_type = file
log_level = info
log_path = /tmp/pgprtdbg.log

[primary]
host = localhost
port = 5432
```

In our main section called `[pgprtdbg]` we setup [**pgprtdbg**](https://github.com/jesperpedersen/pgprtdbg) to listen on all network addresses.
Logging will be performed at `info` level and put in a file called `/tmp/pgprtdbg.log`.

Next we create a section called `[primary]` which has the information about our PostgreSQL instance. In this case it is running on localhost on port 5432.

Finally, you should be able to obtain the version of [**pgprtdbg**](https://github.com/jesperpedersen/pgprtdbg). Cheers!

#### Start pgprtdbg

``` sh
pgprtdbg -c pgprtdbg.conf
```

#### Stop pgprtdbg

You stop **pgprtdbg** by pressing Ctrl-C in the terminal


## Basic git guide

Here are some links that will help you

* [How to Squash Commits in Git](https://www.git-tower.com/learn/git/faq/git-squash)
* [ProGit book](https://github.com/progit/progit2/releases)

### Start by forking the repository

This is done by the "Fork" button on GitHub.

### Clone your repository locally

This is done by

```sh
git clone git@github.com:<username>/pgprtdbg.git
```

### Add upstream

Do

```sh
cd pgprtdbg
git remote add upstream https://github.com/jesperpedersen/pgprtdbg.git
```

### Do a work branch

```sh
git checkout -b mywork main
```

### Make the changes

Remember to verify the compile and execution of the code

### AUTHORS

Remember to add your name to

```
AUTHORS
```

in your first pull request

### Multiple commits

If you have multiple commits on your branch then squash them

``` sh
git rebase -i HEAD~2
```

for example. It is `p` for the first one, then `s` for the rest

### Rebase

Always rebase

``` sh
git fetch upstream
git rebase -i upstream/main
```

### Force push

When you are done with your changes force push your branch

``` sh
git push -f origin mywork
```

and then create a pull requests for it

### Repeat

Based on feedback keep making changes, squashing, rebasing and force pushing

### Undo

Normally you can reset to an earlier commit using `git reset <commit hash> --hard`. 
But if you accidentally squashed two or more commits, and you want to undo that, 
you need to know where to reset to, and the commit seems to have lost after you rebased. 

But they are not actually lost - using `git reflog`, you can find every commit the HEAD pointer
has ever pointed to. Find the commit you want to reset to, and do `git reset --hard`.
