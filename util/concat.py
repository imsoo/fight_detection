import glob
import ffmpeg

clusters = glob.glob("./Cluster*")

ffmpeg_path = "C:\\ffmpeg\\bin\\ffmpeg.exe"
output_path = ".\\output\\"
output_count = 1
for cl in clusters:
    cameras = glob.glob(cl + "\\Cam*")
    for ca in cameras:
        subjects = glob.glob(ca + "\\S*")
        for su in subjects:
            targets = glob.glob(su + "\\A04\\R03\\*.pgm")
            targets[0] = targets[0].replace("000.pgm", "%3d.pgm")
            in_stream = ffmpeg.input(targets[0])
            in_stream.output(output_path + str(output_count).zfill(5) + ".mp4", r = 25, pix_fmt="yuv420p").run(ffmpeg_path, overwrite_output=True)
            output_count += 1