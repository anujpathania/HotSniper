import sim

pr = 0
app_map = {}

class Perf:
    def setup(self, args):
        global pr
        global app_map

        args = (args or '').split(':')
        pr = int(args[0])

        sim.util.register_command(0x126, Perf.hook_set_app)

        for app in range(0,10):
            sim.stats.register('scheduler', app, 'app_code', self.get_app_code)

        if(pr != 0):
            sim.util.register_command(0x125, Perf.hook_perforation_rate_stub)
            return
        else:
            sim.util.register_command(0x125, Perf.hook_perforation_rate)

    @staticmethod
    def decomp_value(value):
        a = value & 0x000000FF
        b = value >> 16

        return (a, b)

    def get_app_code(self, objectName, index, metricName):
        global app_map
        for key, value in app_map.items():
            if key == index:
                return value
            
        return 0 #long(0)

    @staticmethod
    def hook_set_app(core, id):
        global app_map

        app_id, app_code = Perf.decomp_value(id)
        app_map[app_id] = app_code

        print("[MAGIC]: setting app: {} to be an instance of {}".format(app_id, app_code))

    @staticmethod
    def hook_perforation_rate(core, id):
        loop_id, app_id = Perf.decomp_value(id)

        pr = sim.stats.get("scheduler", loop_id, "{}_perforation_rate".format(app_id))
        
        print("[MAGIC] app: ({}), loop_id: {} retrieved pr: {}".format(app_id, loop_id, pr))
        
        return pr

    @staticmethod
    def hook_perforation_rate_stub(core, id):
        global pr
        return pr
    
sim.util.register(Perf())