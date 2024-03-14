#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#define FIGHTER 0
#define MAGE 1

// added rpg_character struct in task_struct in sched, all the data needed is stored there.

int sys_rpg_create_character(int cclass){
    if (cclass != 0 && cclass != 1) {
        return -EINVAL;
    } else if (current->character.party.next != NULL) {
        return -EEXIST;
    }

    INIT_LIST_HEAD(&current->character.party);
    current->character.cclass = cclass;
    current->character.level = 1;
    return 0;
}

#define ORC 0
#define DEMON 1

int sys_rpg_fight(int type, int level){
    if ((type != 0 && type != 1) || (level < 0) || (current->character.party.prev == NULL)) {
        return -EINVAL;
    }
    int sum_strength = 0;
    struct list_head *pos;
    struct rpg_character *entry;

    pos = &current->character.party;
    do {
        entry = list_entry(pos, struct rpg_character, party);
        int cclass = entry->cclass;
        if (cclass == type) { // mage and demon / orc and fighter are the same value
            sum_strength += 2 * entry->level;
        } else {
            sum_strength += 1 * entry->level;
        }
        pos = pos->next;
    } while (pos != &current->character.party);

    if (sum_strength >= level) {
        pos = &current->character.party;
        do {
            entry = list_entry(pos, struct rpg_character, party);
            entry->level++;
            pos = pos->next;
        } while (pos != &current->character.party);
    } else {
        pos = &current->character.party;
        do {
            entry = list_entry(pos, struct rpg_character, party);
            entry->level = (entry->level - 1 < 0) ? 0 : entry->level - 1;  
            pos = pos->next;
        } while (pos != &current->character.party);
    }
    return !!(sum_strength >= level);
}

struct rpg_stats {
    int cclass;
    int level;
    int party_size;
    int fighter_levels;
    int mage_levels;
};

int sys_rpg_get_stats(struct rpg_stats *stats){
    if (stats == NULL || current->character.party.prev == NULL) {
        return -EINVAL;
    }

    struct rpg_stats out;
    out.cclass = current->character.cclass;
    out.level = current->character.level;

    int party_size = 0;
    int fighter_levels = 0;
    int mage_levels = 0;

    struct list_head *pos;
    struct rpg_character *entry;

    pos = &current->character.party;
    do {
        entry = list_entry(pos, struct rpg_character, party);
        party_size++;
        if (entry->cclass == MAGE) {
            mage_levels += entry->level;
        } else {
            fighter_levels += entry->level;
        }
        pos = pos->next;
    } while (pos != &current->character.party);

    out.party_size = party_size;
    out.fighter_levels = fighter_levels;
    out.mage_levels = mage_levels;

    if (copy_to_user(stats, &out, sizeof(out)) > 0) {
        return -EFAULT;
    } else {
        return 0;
    }
}

int sys_rpg_join(pid_t player){
    task_t *player_task = find_task_by_pid(player);
    if (!player_task) {
        return -ESRCH;
    } else if (current->character.party.prev == NULL || player_task->character.party.prev == NULL) {
        return -EINVAL;
    }
    list_del(&current->character.party);
    list_add(&current->character.party, &player_task->character.party);
    return 0;
}
