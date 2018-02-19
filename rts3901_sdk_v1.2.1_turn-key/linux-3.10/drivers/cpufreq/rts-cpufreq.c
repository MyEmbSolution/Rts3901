#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <asm/clock.h>
#include <asm/idle.h>
#include <asm/time.h>
#include <linux/clk/rts_cpu_clk.h>

static struct cpufreq_frequency_table *rts_freq_table;

static struct clk *rts_cpu_clk;
static DEFINE_MUTEX(rts_cpu_lock);

static int rts_cpu_freq_notifier(struct notifier_block *nb,
					unsigned long val, void *data)
{
	struct cpufreq_freqs *freqs = (struct cpufreq_freqs *)data;

	if (val == CPUFREQ_POSTCHANGE) {
		rlx_hpt_frequency = freqs->new * 1000;
		rlx_clocksource_update();
		rlx_clockevent_update();
	}

	return 0;
}

static struct notifier_block rts_cpufreq_notifier_block = {
	.notifier_call = rts_cpu_freq_notifier,
};

static int rts_cpufreq_verify(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, rts_freq_table);
}

static unsigned int rts_cpufreq_get(unsigned int cpu)
{
	return clk_get_rate(rts_cpu_clk) / 1000;
}

static int rts_cpufreq_target(struct cpufreq_policy *policy,
			unsigned int target_freq,
			unsigned int relation)
{
	unsigned int idx;
	struct cpufreq_freqs freqs;

	mutex_lock(&rts_cpu_lock);

	cpufreq_frequency_table_target(policy, rts_freq_table, target_freq,
			relation, &idx);

	freqs.new = rts_freq_table[idx].frequency;
	freqs.old = rts_cpufreq_get(0);
	if (freqs.old == freqs.new)
		goto out;

	cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);

	rts_set_cpu_clk(rts_cpu_clk, freqs.new * 1000);

	cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);

out:
	mutex_unlock(&rts_cpu_lock);

	return 0;
}

static int rts_cpufreq_init(struct cpufreq_policy *policy)
{
	cpufreq_frequency_table_cpuinfo(policy, rts_freq_table);
	cpufreq_frequency_table_get_attr(rts_freq_table, policy->cpu);
	policy->cur = rts_cpufreq_get(0);

	policy->cpuinfo.transition_latency = 10 * 1000;

	return 0;
}

static int rts_cpufreq_exit(struct cpufreq_policy *policy)
{
	cpufreq_frequency_table_cpuinfo(policy, rts_freq_table);

	return 0;
}

static struct freq_attr *rts_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver rts_cpufreq_driver = {
	.owner = THIS_MODULE,
	.name = "rts",
	.verify = rts_cpufreq_verify,
	.target = rts_cpufreq_target,
	.get = rts_cpufreq_get,
	.init = rts_cpufreq_init,
	.exit = rts_cpufreq_exit,
	.attr = rts_cpufreq_attr,
};

static int rts_cpufreq_platform_probe(struct platform_device *pdev)
{
	rts_cpu_clk = clk_get(NULL, "cpu_ck");
	if (IS_ERR(rts_cpu_clk))
		return PTR_ERR(rts_cpu_clk);

	rts_freq_table = dev_get_platdata(&pdev->dev);

	cpufreq_register_notifier(&rts_cpufreq_notifier_block,
			CPUFREQ_TRANSITION_NOTIFIER);

	return cpufreq_register_driver(&rts_cpufreq_driver);
}

static int rts_cpufreq_platform_remove(struct platform_device *pdev)
{
	cpufreq_unregister_driver(&rts_cpufreq_driver);
	cpufreq_unregister_notifier(&rts_cpufreq_notifier_block,
			CPUFREQ_TRANSITION_NOTIFIER);
	clk_put(rts_cpu_clk);
	rts_cpu_clk = NULL;

	return 0;
}

static struct platform_driver rts_cpufreq_platform_driver = {
	.probe = rts_cpufreq_platform_probe,
	.remove = rts_cpufreq_platform_remove,
	.driver = {
		.name = "rts-cpu-dvfs",
		.owner = THIS_MODULE,
	},
};
module_platform_driver(rts_cpufreq_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek RTS cpufreq driver");
