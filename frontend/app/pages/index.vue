<script setup lang="ts">
import {navigateTo} from "nuxt/app";

const { user } = useAuth()

await navigateTo(`/login`)
</script>

<template>

</template>

<style scoped>

</style>