# r.in.gvar

r.in.gvar is a grass module that reads GOES gvar data into a grass
mapset.  The data can be read either as a stream or a file.  

Be sure to look at the parameters you can include on the command-line.
Because we use a GOES receiver from automated sciences, who add a
small header, there is a flag for those particular formats.

It's important that you use a patched version of the the proj library,
so that you can convert these data to a more easily used projection.
Search on github for proj-goes-patch.

See the main README for compilation instructions.



