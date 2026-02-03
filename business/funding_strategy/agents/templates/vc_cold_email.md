Subject: {{ company.name }} — {{ pitch_hook }}

Hi {{ contact_name | default("team") }},

{{ opening_line }}

{{ company.name }} is {{ one_liner }}.

Key metrics:
{{ metrics_block }}

{{ ask }}

{{ closing }}

{{ founder.name }}
{{ founder.title }}
{{ founder.email }}{% if founder.phone %}
{{ founder.phone }}{% endif %}
{{ company.website }}
