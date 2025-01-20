#Web frienly RTI format

<!-- binary files are opaque to the users, harder to parse, and in generally not suited
for the web, where bandwith and latency limits performances. The legacy files were not designed
taking into account  web constraint -->

<!-- RTI coefficients are efficiently compressed using images, usually jpg -->

<!-- relight uses a simple structure composed of a json and a few images, this  fine if
the images are small for large images, using pyramidal approaches (deepzoom for example)
is advised. -->

<!-- pyramidal image management generate a large number of files, this can create problem managing
and transfering the files, a workaround is tarzoom which concatenate all the images in a single file, 
and takes advantage of the 206 range request ability of http protocol to request a segment of a file,
take into account that some http server do not support 206 protocol -->

<!-- another problem is related to the number of http requests (one per image, up to 9 per tile), 
some hosting have anti denial of service and limit the number of requests per second, and
fast connections can trigger the limit. Itarzoom compact all the images for the same tile in a single request -->
