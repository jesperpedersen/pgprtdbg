\newpage

# Quick start

Make sure that [**pgprtdbg**][pgprtdbg] is installed and in your path by using `pgprtdbg -?`. You should see

``` console
pgprtdbg 0.4.0
  PostgreSQL protocol debugging

Usage:
  pgprtdbg [ -c CONFIG_FILE ] [ -d ]

Options:
  -c, --config CONFIG_FILE Set the path to the pgprtdbg.conf file
  -d, --daemon             Run as a daemon
  -V, --version            Display version information
  -?, --help               Display help
```

If you encounter any issues following the above steps, you can refer to the **Installation** chapter to see how to install or compile pgprtdbg on your system.

## Configuration

Lets create a simple configuration file called `pgprtdbg.conf` with the content

``` ini
[pgprtdbg]
host = localhost
port = 2346

output = pgprtdbg.out

log_type = file
log_level = info
log_path = /tmp/pgprtdbg.log

[primary]
host = localhost
port = 5432
```

In our main section called `[pgprtdbg]` we setup [**pgprtdbg**][pgprtdbg] to listen on all network addresses. Logging will be performed at `info` level and put in a file called `/tmp/pgprtdbg.log`.

Next we create a section called `[primary]` which has the information about our [PostgreSQL][postgresql] instance. In this case it is running on `localhost` on port `5432`.

See the **Configuration** charpter for all configuration options.

## Running

We will run [**pgprtdbg**][pgprtdbg] using the command

``` sh
pgprtdbg -c pgprtdbg.conf
```

If this doesn't give an error, then we are ready to do backups.

[**pgprtdbg**][pgprtdbg] is stopped by pressing Ctrl-C (`^C`) in the console where you started it, or by sending the `SIGTERM` signal to the process using `kill <pid>`.

## Next Steps

Next steps in improving pgprtdbg's configuration could be

* Update `pgprtdbg.conf` with the required settings for your system

See [Configuration][configuration] for more information on these subjects.
