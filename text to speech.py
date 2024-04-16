import subprocess

common_words = open("common three letter words.txt")
for word in common_words:
    word = word.strip()
    output = subprocess.run(
        ["tts", "--text", "%s." % word, "--out_path", "audio_files/%s.wav" % word],
        capture_output=True,
    )
    if output.returncode != 0:
        print("error: tts with %s" % word)
        break

    print("finished %s" % word)
