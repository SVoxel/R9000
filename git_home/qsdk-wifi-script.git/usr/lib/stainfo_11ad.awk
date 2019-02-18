BEGIN {
    m = 0
    n = 0
}

flag = 0

$1 ~ /Station/ {
        station[m] = $2
        m = m + 1
    }

$1 ~ /tx/ && $2 ~ /bitrate:/ {
        rate[n] = $3
        n = n + 1
    }

END {
    for(i = 0; i < m ; i++){
        printf "%s\t%sMbps\t0\n", station[i], rate[i]
    }
}
