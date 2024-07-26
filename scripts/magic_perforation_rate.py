import os
import sim

pr = []

class Perforation:
    def setup(self, args):
        global pr
        # global app_map

        args = (args or '').split(':')

        # 'pr_1,pr_2,...,pr_n'
        pr = [e for e in args[0].split(',')]
        
        sim.util.register_command(0x126, Perforation.hook_set_app)

        sim.util.register_command(0x127, Perforation.hook_log)

        if(len(pr) != 0):
            sim.util.register_command(0x125, Perforation.hook_m_perforation_rate_stub)
            return
        else:
            sim.util.register_command(0x125, Perforation.hook_perforation_rate)

    @staticmethod
    def decomp_value(value):
        a = value & 0x000000FF
        b = value >> 16

        return (a, b)

    @staticmethod
    def hook_set_app(core, id):
        # global app_map

        app_id, app_code = Perforation.decomp_value(id)
        # app_map[app_id] = app_code

        results_path = os.getcwd()
        file = open(os.path.join(results_path, 'app_mapping.txt'), "w")
        file.write("{}, {}".format(app_id, app_code))
        file.close()

        print("[MAGIC]: setting app: {} to be an instance of {}".format(app_id, app_code))


    @staticmethod
    def hook_log(core, id):
        
        # a, b = Perf.decomp_value(id)
        # print("[MAGIC]: a: {} b: {}".format(a, b))
        print("[MAGIC]: value: {}".format(id))
        return id

    @staticmethod
    def hook_perforation_rate(core, id):
        loop_id, app_id = Perforation.decomp_value(id)

        pr = sim.stats.get("scheduler", loop_id, "{}_perforation_rate".format(app_id))
        
        print("[MAGIC] app: ({}), loop_id: {} retrieved pr: {}".format(app_id, loop_id, pr))
        
        return pr

    @staticmethod
    def hook_perforation_rate_stub(core, id):
        global pr
        return pr
    
    @staticmethod
    def hook_m_perforation_rate_stub(core, id):
        global pr

        loop_id, app_id = Perforation.decomp_value(id)
        try:
            print("[MAGIC] **stub** app: ({}), loop_id: {} retrieved pr: {}".format(app_id, loop_id, pr[loop_id]))
            return long(pr[loop_id])
        except IndexError as e:
            print("[MAGIC] **stub**, WARNING: index out of range ({}, {})".format(loop_id, pr[0]))
            return long(pr[0])
    
    
sim.util.register(Perforation())
