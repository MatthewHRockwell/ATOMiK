{{/*
Common labels
*/}}
{{- define "atomik.labels" -}}
app: atomik
component: kmod
chart: {{ .Chart.Name }}-{{ .Chart.Version }}
release: {{ .Release.Name }}
{{- end }}

{{/*
Selector labels (stable across upgrades)
*/}}
{{- define "atomik.selectorLabels" -}}
app: atomik
component: kmod
{{- end }}

{{/*
Namespace — allow override
*/}}
{{- define "atomik.namespace" -}}
{{ .Values.namespaceOverride | default .Release.Namespace }}
{{- end }}
