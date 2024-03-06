import sim

pr = 0
class Perf:
    def setup(self, args):
        global pr
        print(args)
        args = (args or '').split(':')
        pr = int(args[0])

    @staticmethod
    def hook_perforation_rate(core, a):
        sniper_pr = sim.stats.get("scheduler", 0, "perforation_rate")
        global pr
        print("[MAGIC] recieved perforation rate: {perforation_rate}".format(perforation_rate=pr))
        
        # print("script: {} == sniper: {}".format(type(pr), type(sniper_pr)))
        return pr


sim.util.register(Perf())
sim.util.register_command(0x125, Perf.hook_perforation_rate)
