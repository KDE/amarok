#!/bin/bash

function header {
        echo -e "\033[0;34m==>\033[0;0;1m $1 \033[0;0m"
}

function middle {
        echo -e "\033[0;33m==>\033[0;0;1m $1 \033[0;0m"
}

function die {
    exit_code=$?
    echo $1
    exit $exit_code
}
#!/bin/bash

function header {
        echo -e "\033[0;34m==>\033[0;0;1m $1 \033[0;0m"
}

function middle {
        echo -e "\033[0;33m==>\033[0;0;1m $1 \033[0;0m"
}

function die {
    exit_code=$?
    echo $1
    exit $exit_code
}