## Install pgprtdbg

This tutorial will show you how to do a simple installation of [**pgprtdbg**](https://github.com/jesperpedersen/pgprtdbg).

At the end of this tutorial you will have a protocol proxy to a PostgreSQL cluster.

Please note that inside the brackets at the end of each step it's the user account
you should be using, switch the account when needed.

### Preface

This tutorial assumes that you have an installation of PostgreSQL 12+ and [**pgprtdbg**](https://github.com/pgprtdbg/pgprtdbg).

For RPM based distributions such as Fedora and RHEL you can add the
[PostgreSQL YUM repository](https://yum.postgresql.org/) and do the install via

```
dnf -qy module disable postgresql
dnf install -y postgresql13 postgresql13-server pgprtdbg
```

### Initialize cluster
```
export PATH=/usr/pgsql-12/bin:$PATH
initdb /tmp/pgsql
```

(`postgres` user)

### Remove default access

Remove

```
host    all             all             127.0.0.1/32            trust
host    all             all             ::1/128                 trust
host    replication     all             127.0.0.1/32            trust
host    replication     all             ::1/128                 trust
```

from `/tmp/pgsql/pg_hba.conf`

(`postgres` user)

### Add access for users and a database

Add

```
host    mydb             myuser          127.0.0.1/32            md5
host    mydb             myuser          ::1/128                 md5
```

to `/tmp/pgsql/pg_hba.conf`

Remember to check the value of `password_encryption` in `/tmp/pgsql/postgresql.conf`
to setup the correct authentication type.

(`postgres` user)

### Start PostgreSQL

```
pg_ctl -D /tmp/pgsql/ start
```

(`postgres` user)

### Add users and a database

```
createuser -P myuser
createdb -E UTF8 -O myuser mydb
```

with `mypass` as the password.

(`postgres` user)

### Verify access

For the user `myuser` using `mypass` as the password

```
psql -h localhost -p 5432 -U myuser mydb
\q
```

(`postgres` user)

### Add pgprtdbg user

```
sudo su -
useradd -ms /bin/bash pgprtdbg
passwd pgprtdbg
exit
```

(`postgres` user)

### Create pgprtdbg configuration

Switch to the pgprtdbg user

```
sudo su -
su - pgprtdbg
```

If you see an error saying `error while loading shared libraries: libpgprtdbg.so.0: cannot open shared object` running 
the above command. you may need to locate where your `libpgprtdbg.so.0` is. It could be in `/usr/local/lib` or `/usr/local/lib64`
depending on your environment. Add the corresponding directory into `/etc/ld.so.conf`, or alternatively, create a file
called `pgprtdbg_shared_library.conf` under `/etc/ld.so.conf.d/`, and add your directory into it. Remember to run `ldconfig` to make the change effective

Create the `pgprtdbg.conf` configuration
```
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

and press `Ctrl-D`

(`pgprtdbg` user)

### Start pgprtdbg

```
pgprtdbg -c pgprtdbg.conf
```

(`pgprtdbg` user)

### Stop pgprtdbg

You stop **pgprtdbg** by pressing Ctrl-C in the terminal

(`pgprtdbg` user)
