import sim

def hook_perforation_rate(core, a):
    print("core: {core}, a: {a}".format(core=core, a=a))
    
    if a % 3 == 0:
        return 1
    else:
        return 2
    

sim.util.register_command(0x125, hook_perforation_rate)
