import sim

pr = 0
class Perf:
    def setup(self, args):
        global pr
        print(args)
        args = (args or '').split(':')
        pr = int(args[0])
        
        if(pr != 0):
            sim.util.register_command(0x125, Perf.hook_perforation_rate_stub)
            return

        sim.util.register_command(0x125, Perf.hook_perforation_rate)

    @staticmethod
    def hook_perforation_rate(core, a):
        pr = sim.stats.get("scheduler", 0, "perforation_rate")
        
        print("[MAGIC] recieved perforation rate: {perforation_rate}".format(perforation_rate=pr))
        
        return pr
    
    @staticmethod
    def hook_perforation_rate_stub(core, a):
        global pr
        return pr



sim.util.register(Perf())
