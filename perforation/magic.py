
def bit_shift_example(id):
    loop_id = id & 0x0000FFFF
    app_id = id >> 16

    return (app_id, loop_id)

app_id = 42
loop_id = 24

tot_id = 0x00000000
tot_id = tot_id | (app_id << 16)
tot_id = tot_id | loop_id


ret, ret2 = bit_shift_example(tot_id)

print("{}, {}".format(ret, ret2))