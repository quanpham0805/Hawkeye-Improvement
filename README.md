# Improvements on Hawkeye Cache Replacement Policy

## Modification from Hawkeye

Several modifications:
- Added Expected Hit Count Hit History Table (HHT) to break tie between lines that are not cache-averse but have the highest RRIP value.
- Added perceptron-based predictor on global cache prediction history (in one of the old commit)
- Added perceptron-based predictor on local cache prediction history (latest commit)

## How to run

This repo contains only the small trace files. For bigger traces, they can be found here in this Googe Cloud bucket: [gs://cs450-trace/](https://console.cloud.google.com/storage/browser/cs450-trace)

To download, you can run
```bash
cd trace && mkdir big &&cd big
gcloud storage cp gs://cs450-trace
```

To get results, run these commands sequentially:

```bash
./scripts/compile_all.sh
./scripts/generate_results.sh big 100000000
./scripts/gencsv.sh
./scripts/gen_graph_csv.sh small
```

These commands will generate 2 files for 2 config corresponding to without and with prefetching, called `plt1.csv` and `plt2.csv`. They can be uploaded to Google Spreadsheet to create graphs based on the IPC or the Miss Rate.
