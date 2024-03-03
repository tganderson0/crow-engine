def get_average_for_file(filename):
  with open(filename) as file:
    vals = [float(ms) for ms in file.readlines()]
    print(f"Average ms for {filename}: {round(sum(vals) / len(vals), 4)}")

if __name__ == '__main__':
  for file in [
    "renderTimes_on_remote_complex.txt",
    "renderTimes_only_self.txt",
    "renderTimes_simple_only_self.txt",
    "renderTimes_simple_with_remote.txt",
    "renderTimes_with_remote.txt",
    "encodingTimes.txt",
    "networkTransferTimes.txt"
  ]:
    get_average_for_file(file)