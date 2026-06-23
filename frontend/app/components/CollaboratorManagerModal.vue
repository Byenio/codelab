<script setup lang="ts">
import { ref, computed } from 'vue'
import { useApi } from '~/composables/useApi'

const props = defineProps<{
  repoId: number
}>()

const isOpen = defineModel<boolean>()

const toast = useToast()

const { data: collaborators, refresh } = useApi<{ id: number; username: string }[]>(`/api/v1/repositories/${props.repoId}/collaborators`)

const newUsername = ref('')

const addCollaborator = async () => {
  if (!newUsername.value) return
  try {
    await useApi(`/api/v1/repositories/${props.repoId}/collaborators`, {
      method: 'POST',
      body: { username: newUsername.value }
    })
    newUsername.value = ''
    await refresh()
    toast.add({ title: 'Collaborator added', color: 'success' })
  } catch (e) {
    toast.add({ title: 'Error', description: 'Failed to add collaborator (not found or already collaborating)', color: 'error' })
  }
}

const removeCollaborator = async (username: string) => {
  if (!confirm(`Remove collaborator "${username}"?`)) return
  try {
    await useApi(`/api/v1/repositories/${props.repoId}/collaborators/${username}`, {
      method: 'DELETE'
    })
    await refresh()
    toast.add({ title: 'Collaborator removed', color: 'success' })
  } catch (e) {
    toast.add({ title: 'Error', description: 'Failed to remove collaborator', color: 'error' })
  }
}
</script>

<template>
  <UModal
    v-model:open="isOpen"
    title="Manage Collaborators"
  >
    <template #body>
      <div class="space-y-6">
        <div>
          <h4 class="font-medium text-sm mb-2">
            Existing Collaborators
          </h4>
          <ul
            v-if="collaborators && collaborators.length"
            class="divide-y divide-zinc-200 dark:divide-zinc-800 border dark:border-zinc-800 rounded-md"
          >
            <li
              v-for="user in collaborators"
              :key="user.id"
              class="flex justify-between items-center p-2 text-sm"
            >
              <span>{{ user.username }}</span>
              <UButton
                color="red"
                variant="ghost"
                size="xs"
                icon="i-heroicons-user-minus"
                @click="removeCollaborator(user.username)"
              />
            </li>
          </ul>
          <p
            v-else
            class="text-sm text-zinc-500"
          >
            No collaborators.
          </p>
        </div>

        <div>
          <h4 class="font-medium text-sm mb-2">
            Add Collaborator
          </h4>
          <div class="flex gap-2">
            <UInput
              v-model="newUsername"
              placeholder="Exact username"
              class="flex-1"
            />
            <UButton
              icon="i-heroicons-user-plus"
              @click="addCollaborator"
            >
              Add
            </UButton>
          </div>
        </div>
      </div>
    </template>
  </UModal>
</template>
