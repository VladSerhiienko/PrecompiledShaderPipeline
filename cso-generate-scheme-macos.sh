
rm -rf ./include/generated
rm -rf ./include/mutable_generated
./bin/macOS/flatc -o ./include/generated -c ./schemes/cso.fbs
./bin/macOS/flatc -o ./include/mutable_generated -c ./schemes/cso.fbs --gen-mutable
