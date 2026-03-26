import sys

def extract_code(diff_path):
    with open(diff_path, 'r') as f:
        lines = f.readlines()
    
    out = []
    in_diff = False
    for line in lines:
        if line.startswith('+++ b/nx47_vesu_kernel_v2.py'):
            in_diff = True
            continue
        if in_diff:
            if line.startswith('@@'):
                continue
            if line.startswith('+'):
                out.append(line[1:])
            elif line.startswith('-'):
                continue
            elif line.startswith(' '):
                out.append(line[1:])
            elif line.startswith('diff --git'):
                break
    return "".join(out)

code = extract_code('attached_assets/Pasted-diff-git-a-nx47-vesu-kernel-v2-py-b-nx47-vesu-kernel-v2_1771527598182.txt')
with open('nx47_vesu_kernel_v2.py', 'w') as f:
    f.write(code)
