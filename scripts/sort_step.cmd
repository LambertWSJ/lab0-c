
b list_sort.c:237
b list_sort.c:249
b qtest.c:634

p $cnt=1
# divide
command 1
eval "set $png=\"./sort_step/%d_divide.png\"", $cnt
dump_sort pending -o $png
p $cnt += 1
c
end

# merge
command 2
eval "set $png=\"./sort_step/%d_merge.png\"", $cnt
dump_sort pending -o $png
p $cnt += 1
c
end

# final
command 3
eval "set $png=\"./sort_step/%d_list.png\"", $cnt
dump_sort current->q -type origin -o $png
p $cnt += 1
c
end
