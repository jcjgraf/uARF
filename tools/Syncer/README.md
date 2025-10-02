# Syncer

Synchronize repositories or general directories to a remote location. Optionally, run as a daemon to automatically watch for changes. Synchronization behavior is defined via configuration files.

## Usage

1. Create a configuration file
    - Optionally place it in `$XDG_CONFIG_HOME/syncer`.
    - Config files are interpreted as Bash scripts, so you can compute file lists dynamically.
    - Refer to `syncer.sh --help` for detailed options.
    - Example configurations can be found in [uarf.misc].

1. Define a configuration file, optionally putting it in `$XDG_CONFIG_HOME/syncer`
    - Config is interpreted as a bash file, so you can compute these file lists
    - See `syncer.sh --help` for further information
    - See [uarf.misc] for an example
2. Run Syncer

    ```bash
    syncer.sh <CONFIG_NAME_OR_PATH>
    ```

    - If the config is in `$XDG_CONFIG_HOME/syncer`, providing only the config name is sufficient.
    - Use `--daemon` to continuously watch and sync changes in the background.
    - See `syncer.sh --help` for all flags and options.

## Example

```bash
REMOTE_NODE=zen4 syncer.sh -d uarf.conf
```
