#! /bin/sh
cd ..
make um > /dev/null
cd - > /dev/null
make writetests > /dev/null
./writetests > /dev/null
testFiles=$(ls $2 | grep '\.um$')

for testFile in $testFiles ; do
    testName=$(echo $testFile | sed -E 's/(.*).um/\1/')
    testOutput="None"
    refOutput="None"
    if [ -f "${testName}.1" ] ; then
        actualOutput=$(cat ${testName}.1)
        if [ -f "${testName}.0" ] ; then
            testOutput=$(../um $testFile < "${testName}.0")
            refOutput=$(um $testFile < "${testName}.0")
        else
            # echo "Made it here"
            testOutput=$(../um $testFile < "/dev/null")
            refOutput=$(um $testFile < "/dev/null")
            # echo "$testOutput"
        fi
        if [[ "$testOutput" != "$actualOutput" ]] ; then
            echo "Generated output for test ${testName} and ${testName}.1 are different"
            echo "  UM output: ${testOutput}"
            echo "  Correct output: ${actualOutput}"
        fi
        if [[ "$testOutput" != "$refOutput" ]] ; then
            echo "Generated output for test ${testName} and reference output are different"
            echo "  UM output: ${testOutput}"
            echo "  Reference output: ${refOutput}"
        fi
    else
        if [ -f "${testName}.0" ] ; then
            testOutput=$(../um $testFile < "${testName}.0")
            refOutput=$(um $testFile < "${testName}.0")
        else
            testOutput=$(../um $testFile < "/dev/null")
            refOutput=$(um $testFile < "/dev/null")
        fi
        if [[ "$testOutput" != "" ]] ; then
            echo "$testName has output when no output was expected!"
            echo "  UM output: ${testOutput}"
        fi
        if [[ "$testOutput" != "$refOutput" ]] ; then
            echo "$testName (which should not have output) does not match the reference!"
            echo "  UM output: ${testOutput}"
            echo "  Reference output: ${refOutput}"
        fi
    fi
done