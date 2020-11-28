#! /bin/sh
cd ..
make um > /dev/null
cd - > /dev/null
make writetests > /dev/null
./writetests > /dev/null
testFiles=$(ls $2 | grep '\.um$')
noLeakString="All heap blocks were freed -- no leaks are possible"
valgrindOutputFile="valgrindOutput.txt"

for testFile in $testFiles ; do
    testName=$(echo $testFile | sed -E 's/(.*).um/\1/')
    if [ -f "${testName}.0" ] ; then
        valgrind ../um $testFile < "${testName}.0" > /dev/null >& $valgrindOutputFile
    else
        valgrind ../um $testFile < /dev/null > /dev/null >& $valgrindOutputFile
    fi
    testOutput=$(cat $valgrindOutputFile)
    if [[ "$testOutput" != *"$noLeakString"* ]] ; then
        echo "A leak occurred while running ${testFile}."
    fi
done

rm $valgrindOutputFile