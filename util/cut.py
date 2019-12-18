import glob
import ffmpeg

info_file_name = "./info.txt"

in_files = glob.glob("./*00_*.mp4")
in_file_streams = []
for file in in_files:
    in_file_streams.append(ffmpeg.input(file))

out_file_names = ["1.mp4", "2.mp4", "3.mp4", "4.mp4"]
out_file_streams = [None] * len(out_file_names)
init_flags = [False] * len(out_file_names)
filestream = open(info_file_name, "r")
for line in filestream:
    values = line[:-1].split(",")
    action_type = int(values[0])
    start = int(values[1])
    end = int(values[2])
    for in_stream in in_file_streams:
        if init_flags[action_type] == False:
            init_flags[action_type] = True
            out_file_streams[action_type] = ffmpeg.concat(in_stream.trim(start=start, end=end).setpts('PTS-STARTPTS'))
        else:
            out_file_streams[action_type] = ffmpeg.concat(out_file_streams[action_type], in_stream.trim(start=start, end=end).setpts('PTS-STARTPTS'))


for i in range(len(out_file_names)):
    if i == 0:
        continue
    if init_flags[i] == False:
        continue
    out = ffmpeg.output(out_file_streams[i], out_file_names[i], s='720x480')
    ffmpeg.run(out, 'C:\\ffmpeg\\bin\\ffmpeg.exe')