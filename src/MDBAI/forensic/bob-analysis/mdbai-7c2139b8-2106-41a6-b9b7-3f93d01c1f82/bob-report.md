# Rapport d'Analyse Bob (Mode Fallback)

Erreur: Command failed: python3 /home/lvx/LVX/lumvorax2/src/MDBAI/bob-cli/bob_analyzer.py /home/lvx/LVX/lumvorax2/src/MDBAI/forensic/bob-tasks/mdbai-7c2139b8-2106-41a6-b9b7-3f93d01c1f82/bob-task.json
[BOB-CLI] ⚠️  OPENAI_API_KEY manquant - mode fallback
Traceback (most recent call last):
  File "/home/lvx/LVX/lumvorax2/src/MDBAI/bob-cli/bob_analyzer.py", line 192, in <module>
    main()
  File "/home/lvx/LVX/lumvorax2/src/MDBAI/bob-cli/bob_analyzer.py", line 186, in main
    analysis = analyzer.analyze()
               ^^^^^^^^^^^^^^^^^^
  File "/home/lvx/LVX/lumvorax2/src/MDBAI/bob-cli/bob_analyzer.py", line 47, in analyze
    with open(source_file, 'r') as f:
         ^^^^^^^^^^^^^^^^^^^^^^
FileNotFoundError: [Errno 2] No such file or directory: '/home/lvx/LVX/lumvorax2/src/MDBAI/forensic/bob-tasks/mdbai-7c2139b8-2106-41a6-b9b7-3f93d01c1f82/source-code-complete.txt'

