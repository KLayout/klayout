After building all the travis wheels, go to the folder where the wheels were deployed. In my case, for example, `/Users/tlima/Dropbox/Apps/travis-deploy/Builds/klayout/dist-pymod/0.26.0.dev10`. 

Then, run the command 

```bash
travis upload *.whl
```

, which will ask for an username and password related to your account.

After this upload was successful, you can upload the source tarball. Go to klayout's git folder and run `python setup.py sdist`. Inside `dist/`, you'll find a tarball named, e.g., `klayout-0.26.0.dev10.tar.gz`. So just run 

```bash
travis upload klayout-0.26.0.dev10.tar.gz
```
