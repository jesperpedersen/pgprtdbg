# Getting started with pgprtdbg

First of all, make sure that `pgprtdbg` is installed and in your path by
using `pgprtdbg -?`. You should see

```
pgprtdbg 0.3.1
  PostgreSQL protocol debugging

Usage:
  pgprtdbg [ -c CONFIG_FILE ] [ -d ]

Options:
  -c, --config CONFIG_FILE Set the path to the pgprtdbg.conf file
  -d, --daemon             Run as a daemon
  -V, --version            Display version information
  -?, --help               Display help
```

If you don't have `pgprtdbg` in your path see [README](../README.md) on how to
compile and install `pgprtdbg` in your system.

## Configuration

Lets create a simple configuration file called `pgprtdbg.conf` with the content

```
[pgprtdbg]
host = localhost
port = 2346

output = pgprtdbg.out

log_type = console
log_level = info
log_path = 

[primary]
host = localhost
port = 5432
```

In our main section called `[pgprtdbg]` we setup `pgprtdbg` to listen on all
network addresses on port 2346. Logging will be performed at `info` level and
sent to the console. The output of `pgprtdbg` is sent to the `pgprtdbg.out` file.

Next we create a section called `[primary]` which has the information about our
[PostgreSQL](https://www.postgresql.org) instance. In this case it is running
on `localhost` on port `5432`.

We are now ready to run `pgprtdbg`.

See [Configuration](./CONFIGURATION.md) for all configuration options.

## Running

We will run `pgprtdbg` using the command

```
pgprtdbg -c pgprtdbg.conf
```

If this doesn't give an error, then we are ready to connect.

We will assume that we have a user called `test` with the password `test` in our
[PostgreSQL](https://www.postgresql.org) instance. See their
[documentation](https://www.postgresql.org/docs/current/index.html) on how to setup
[PostgreSQL](https://www.postgresql.org), [add a user](https://www.postgresql.org/docs/current/app-createuser.html)
and [add a database](https://www.postgresql.org/docs/current/app-createdb.html).

We will connect to `pgprtdbg` using the [psql](https://www.postgresql.org/docs/current/app-psql.html)
application.

```
psql -h localhost -p 2346 -U test test
```

That should give you a password prompt where `test` should be typed in. You are now connected
to [PostgreSQL](https://www.postgresql.org) through `pgprtdbg`.

`pgprtdbg` will log the protocol interactions in the file `pgprtdbg.out`. The format of the output
file is based on the actual message format type, but has the prefix of

```
[C|S],<COMMAND>
```

where `C` is client, and `S` is the server. The `<COMMAND>` is the message type identifier found in the
[specification](https://www.postgresql.org/docs/devel/protocol-message-formats.html).

`pgprtdbg` is stopped by pressing Ctrl-C (`^C`) in the console where you started it, or by sending
the `SIGTERM` signal to the process using `kill <pid>`.

## Closing

The [pgprtdbg](https://github.com/jesperpedersen/pgprtdbg) community hopes that you find
the project interesting.

Feel free to

* [Ask a question](https://github.com/jesperpedersen/pgprtdbg/discussions)
* [Raise an issue](https://github.com/jesperpedersen/pgprtdbg/issues)
* [Submit a feature request](https://github.com/jesperpedersen/pgprtdbg/issues)
* [Write a code submission](https://github.com/jesperpedersen/pgprtdbg/pulls)

All contributions are most welcome !
