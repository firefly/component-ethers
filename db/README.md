Databases
=========

These databases files are built into header files to provide a
"trusted" source for Ethereum (and ilk) data.


Modifying
---------

These files would get large if *just anyone* were allowed to update them,
as there are very many projects that wish to include their details.

For this reason, the included Databases are meant to be kept lean,
relevant and useful for a broad audience.

However, since each application may have its own requirements, scripts
are provided to simplify developers updating the database and generating
the corresponding header files used by the API.


Network Database
----------------

The Network database stores a mapping of network Chain ID to the
Network Name and Network Token Name.

```
Imports:       networks.json
Export Script: export-networks.mjs

API:
  // Returns the network name for %%chainId%% (or NULL if unknown network)
  const char* ffx_db_getNetworkName(FfxBigInt *chainId);

  // Returns the network token for %%chainId%% (or NULL if unknown network)
  const char* ffx_db_getNetworkToken(FfxBigInt *chainId);
```


