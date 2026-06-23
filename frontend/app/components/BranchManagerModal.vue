<script setup lang="ts">
import { ref, computed } from 'vue'
import { useApi } from '~/composables/useApi'

const props = defineProps<{
  repoId: number
}>()

const isOpen = defineModel<boolean>()

const toast = useToast()

const { data: branches, refresh } = useApi<any[]>(`/api/v1/repositories/${props.repoId}/branches`)

const newBranchName = ref('')
const createSourceBranch = ref('')
const mergeSourceBranch = ref('')
const targetBranch = ref('')

const createBranch = async () => {
  if (!newBranchName.value || !createSourceBranch.value) return
  try {
    await useApi(`/api/v1/repositories/${props.repoId}/branches`, {
      method: 'POST',
      body: { name: newBranchName.value, target_branch: createSourceBranch.value }
    })
    newBranchName.value = ''
    createSourceBranch.value = ''
    await refresh()
    toast.add({ title: 'Branch created', color: 'success' })
  } catch (e) {
    toast.add({ title: 'Error', description: 'Failed to create branch', color: 'error' })
  }
}

const deleteBranch = async (branchName: string) => {
  if (!confirm(`Are you sure you want to delete branch "${branchName}"?`)) return
  try {
    await useApi(`/api/v1/repositories/${props.repoId}/branches/${branchName}`, {
      method: 'DELETE'
    })
    await refresh()
    toast.add({ title: 'Branch deleted', color: 'success' })
  } catch (e) {
    toast.add({ title: 'Error', description: 'Failed to delete branch', color: 'error' })
  }
}

const mergeBranch = async () => {
  if (!mergeSourceBranch.value || !targetBranch.value) return
  try {
    await useApi(`/api/v1/repositories/${props.repoId}/branches/${targetBranch.value}/merge`, {
      method: 'POST',
      body: { source_branch: mergeSourceBranch.value }
    })
    mergeSourceBranch.value = ''
    targetBranch.value = ''
    await refresh()
    toast.add({ title: 'Branch merged successfully', color: 'success' })
  } catch (e) {
    toast.add({ title: 'Merge failed', description: 'Conflicts detected or branch not found', color: 'error' })
  }
}
</script>

<template>
  <UModal
    v-model:open="isOpen"
    title="Manage Branches"
  >
    <template #body>
      <div class="space-y-6">
        <!-- Branch List -->
        <div>
          <h4 class="font-medium text-sm mb-2">
            Existing Branches
          </h4>
          <ul class="divide-y divide-zinc-200 dark:divide-zinc-800 border dark:border-zinc-800 rounded-md">
            <li
              v-for="branch in branches"
              :key="branch.name"
              class="flex justify-between items-center p-2 text-sm"
            >
              <span class="font-mono">
                {{ branch.name }}
                <UBadge
                  v-if="branch.is_head"
                  size="xs"
                >HEAD</UBadge>
              </span>
              <UButton
                color="red"
                variant="ghost"
                size="xs"
                icon="i-heroicons-trash"
                @click="deleteBranch(branch.name)"
              />
            </li>
          </ul>
        </div>

        <!-- Create Branch -->
        <div>
          <h4 class="font-medium text-sm mb-2">
            Create Branch
          </h4>
          <div class="flex gap-2">
            <UInput
              v-model="newBranchName"
              placeholder="New branch name"
              class="flex-1"
            />
            <USelectMenu
              v-model="createSourceBranch"
              :items="branches?.map(b => b.name) || []"
              placeholder="From base"
              class="w-32"
            />
            <UButton @click="createBranch">
              Create
            </UButton>
          </div>
        </div>

        <!-- Merge Branch -->
        <div>
          <h4 class="font-medium text-sm mb-2">
            Local Merge
          </h4>
          <div class="flex gap-2 items-center">
            <USelectMenu
              v-model="mergeSourceBranch"
              :items="branches?.map(b => b.name) || []"
              placeholder="Source"
              class="flex-1"
            />
            <span class="text-zinc-500">into</span>
            <USelectMenu
              v-model="targetBranch"
              :items="branches?.map(b => b.name) || []"
              placeholder="Target"
              class="flex-1"
            />
            <UButton
              color="green"
              @click="mergeBranch"
            >
              Merge
            </UButton>
          </div>
        </div>
      </div>
    </template>
  </UModal>
</template>
