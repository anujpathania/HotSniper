import sim

pr = 0
class Perf:
    def setup(self, args):
        global pr

        args = (args or '').split(':')
        pr = int(args[0])

        if(pr != 0):
            sim.util.register_command(0x125, Perf.hook_perforation_rate_stub)
            return

        sim.util.register_command(0x125, Perf.hook_perforation_rate)

    @staticmethod
    def decomp_id(id):
        loop_id = id & 0x0000FFFF
        app_id = id >> 16

        # return (42, 42)
        return (app_id, loop_id)

    @staticmethod
    def hook_perforation_rate(core, id):
        app_id, loop_id = Perf.decomp_id(id)

        # pr = sim.stats.get("scheduler", app_id, "pr_{}".format(loop_id))
        pr = sim.stats.get("scheduler", 0, "perforation_rate")
        
        print("[MAGIC] (core:{} app:{}, loop:{}) retrieved pr: {}".format(core, app_id, loop_id, pr))
        
        return pr

    @staticmethod
    def hook_perforation_rate_stub(core, id):
        global pr
        return pr
    
sim.util.register(Perf())